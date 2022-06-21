#include "StdAfx.h"

#include "SBFTNode.h"
#include <random>
#include <iomanip>


SBFTNode::SBFTNode(unsigned int id,
				NetworkManager1::Ptr pNetwork,
				DigitalSignatureManager::Ptr pSignatureManager,
				ByteVector vbPrivateKey,
				std::vector<ByteVector>& const vbPublicKeys) :
	Node(id, pNetwork, pSignatureManager, vbPrivateKey, vbPublicKeys),
	m_state(new SBFTState { State::PRE_PREPARE, 0, 0}), // Состояние инициализируется как PREPREPARE, номер view 0, номер текущего запроса 0
	m_pLogger(new NodeLogger(id))
{
	m_transactionPool = std::shared_ptr<std::vector<SBFTDataPackage*>>(new std::vector<SBFTDataPackage*>);
	m_pAddingBlockInProcessMutex.reset(new std::mutex);
}


SBFTNode::~SBFTNode()
{
}


void SBFTNode::operator()()
{
	// Здесь всё аналогично Node, только вместо SimpleDataPackage отправляется 
	// SBFTDataPackage с типом COMMON_MESSAGE, что означает, что он содержит только текстовое сообщение

	std::thread listenThread(&SBFTNode::ListenMessages, this);

	std::mt19937::result_type seed = time(0);
	// (это один из изменяемых для экспериментов параметров)
	auto getRandomInt = std::bind(std::uniform_int_distribution<int>(1, 5), std::mt19937(seed));

	while (!m_bStopped->load())
	{
		std::this_thread::sleep_for(std::chrono::seconds(getRandomInt()));

		SBFTDataPackage* dataToSend{ new SBFTDataPackage(m_id, UINT_MAX, SBFTDataPackage::MessageType::COMMON_MESSAGE) };
		dataToSend->message = "Hello from node with ID " + std::to_string(m_id);
		dataToSend->vbSign = m_pSignatureManager->Sign(dataToSend->GetRawData(), m_vbPrivateKey);

		m_pNetwork->PostData(dataToSend);
	}

	listenThread.join();
}


// Печаетает хеши всех закреплённых блоков
void SBFTNode::PrintTransactionsFromPool() const
{
	std::lock_guard<std::mutex> chainLock(*m_poolMutex);

	std::cout << std::dec << "Node with id " << m_id << ":\n";
	for (Block& block : m_chain->GetChain())
	{
		std::cout << "Block #" << std::dec << block.GetIndex() << " with hash: ";
		for (CryptoPP::byte& hashByte : block.GetHash())
			std::cout << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(hashByte);
		std::cout << std::endl;
	}
	std::cout << std::endl;
}


void SBFTNode::Stop()
{
	m_bStopped->store(true);
}


void SBFTNode::ListenMessages()
{
	// Для имитации задержки (это один из изменяемых для экспериментов параметров)
	std::mt19937::result_type seed = time(0);
	auto getRandomInt = std::bind(std::uniform_int_distribution<int>(12, 25), std::mt19937(seed));

	while (!m_bStopped->load())
	{
		// Имитация задержки (раскомментируется, если нужно)

		//std::this_thread::sleep_for(std::chrono::milliseconds(getRandomInt()));

		// Получение данных, присланных этому узлу (блокирует поток, если данных пока нет)
		SBFTDataPackage* data = dynamic_cast<SBFTDataPackage*>(m_pNetwork->GetData(m_id));

		m_pLogger->LogMessage("Got message of type " + std::to_string(static_cast<int>(data->type)));

		// Если данные не предназначениы ни конкретно этому узлу, ни всем узлам в сети, пакет отбрасывается
		if (!(data->uiDestination == m_id ||
			data->uiDestination == UINT_MAX))
			continue;

		// Если подпись неверна, пакет отбрасывается
		if (!m_pSignatureManager->Verify(data->GetRawData(), data->vbSign, m_vvbPublicKeys[data->uiSender]))
			continue;

		// Если пакет содержит обычноетекстовое сообщение (аналог транзакции)
		if (data->type == SBFTDataPackage::MessageType::COMMON_MESSAGE)
		{
			std::lock_guard<std::mutex> chainLock(*m_poolMutex);
			// Сообщение закидывается в пул
			m_transactionPool->push_back(data);

			// Если сообщений в пуле 10 или больше
			if (m_transactionPool->size() >= 10)
			{
				//if (m_pAddingBlockInProcessMutex->try_lock())
				{
					// В новом потоке начинается создание блока и его закрепление
					std::thread t(&SBFTNode::AddNewBlock, this);
					t.detach();
				}
			}

			continue;
		}

		// Если в пакете не обычное сообщение, обработка производится в зависимости от состояния
		switch (m_state->state)
		{
		case State::PRE_PREPARE:
		{
			// Если пришёл запрос от клиента
			if (data->type == SBFTDataPackage::MessageType::REQUEST)
			{
				// Если прислали REQUEST, но эта нода не Primary, пакет отбрасывается
				if (!(m_state->uiCurrentViewNumber % m_vvbPublicKeys.size() == m_id))
					break;
				else
				{
					// Устанавливается флаг, что запрос получен. Если запрос уже был получен, пакет отбрасывается
					if (!bRequestRecieved)
						bRequestRecieved = true;
					else
						break;

					// Создаётся пакет preprepare (утечка памяти пока не устранена, т.к. непонятно, 
					// в какой момент все потоки завершат обработаку данного пакета, ятобы можно было чистить память)
					//
					// Здесь и далее в пакете заполняются только нужные поля. Предполагается, что пролучатель будет 
					// читать только нужные поля, отталкиваясь от типа пакета
					SBFTDataPackage* prepreparePackage{ new SBFTDataPackage(m_id, UINT_MAX, SBFTDataPackage::MessageType::PREPREPARE) };
					prepreparePackage->pClientRequest = data;
					prepreparePackage->puiSequenceNumber = new unsigned int{ m_state->uiCurrentRequestNumber++ };
					prepreparePackage->puiViewNumber = new unsigned int{ m_state->uiCurrentViewNumber };
					prepreparePackage->vbMessageDigest = data->GetDataHash();
					// Пакет подписывается
					prepreparePackage->vbSign = m_pSignatureManager->Sign(prepreparePackage->GetRawData(), m_vbPrivateKey);

					// Пакет рассылается всем
					m_pNetwork->PostData(prepreparePackage);

					// Бесполезная смена состояния, мы и так в нём
					m_state->state = State::PRE_PREPARE;

					// Сообщение в лог ---------------------------------------------------------------------------------------------------
					m_pLogger->LogMessage("Switched to preprepare");

					break;
				}
			}

			// Если эта нода – клиент, закрепляющий блок и она получила ответ об успещшном закреплении
			else if (data->type == SBFTDataPackage::MessageType::REPLY)
			{
				// Если узел не ждал закрепления, а оно ему пришло, пакет отбрасывается
				if (!m_state->bIsWaitingForReply.first)
					break;

				// Если закреплён тот пакет, который запрашивался
				if (data->pBlock->GetHash() == m_state->bIsWaitingForReply.second->GetHash())
					; // Здесь должно быть нечто, означающее, что всё хорошо и блок закреплён
			}

			else
			{
				// Если сообщение не PREPREPARE, пакет отбрасывается
				if (data->type != SBFTDataPackage::MessageType::PREPREPARE)
					break;

				// Если хеш неверен, пакет отбрасывается
				if (data->pClientRequest->GetDataHash() != data->vbMessageDigest)
					break;

				// Если View не тот, пакет отбрасывается
				if (*data->puiViewNumber != m_state->uiCurrentViewNumber)
					break;

				// Если в логе такого запроса ещё нет, добавляется
				if (m_state->requestLog.find({ *data->puiViewNumber, *data->puiSequenceNumber }) == m_state->requestLog.end())
					m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }] = { 0, 0, data, nullptr, false };
				else
					// Если же в логе всё же есть такой запрос, но присланный пакет не совпадает по параметрам, он отбрасывается
					if (m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->vbMessageDigest != data->vbMessageDigest)
						break;

				// Если порядковый номер запроса не удовлетворяет ограничениям, пакет отбрасывается
				if (!(0 <= *data->puiSequenceNumber <= UINT_MAX))
					break;

				// Создаётся пакет prepare (утечка памяти пока не устранена, т.к. непонятно, 
				// в какой момент все потоки завершат обработаку данного пакета, ятобы можно было чистить память)
				SBFTDataPackage* preparePackage{ new SBFTDataPackage(m_id, UINT_MAX, SBFTDataPackage::MessageType::PREPARE) };
				preparePackage->puiSequenceNumber = new unsigned int{ *data->puiSequenceNumber };
				preparePackage->puiViewNumber = new unsigned int{ *data->puiViewNumber };
				preparePackage->vbMessageDigest = data->vbMessageDigest;
				preparePackage->puiNodeId = new unsigned int{ m_id };
				preparePackage->vbSign = m_pSignatureManager->Sign(preparePackage->GetRawData(), m_vbPrivateKey);

				m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].prepareRequest = preparePackage;

				m_pNetwork->PostData(preparePackage);

				m_state->state = State::PREPARE;

				// Сообщение в лог ---------------------------------------------------------------------------------------------------
				m_pLogger->LogMessage("Switched to prepare");

				break;
			}
		}

		case State::PREPARE:
		{
			// Проверки пакета на валидность, почти аналогично предыдущему состоянию
			if (data->type != SBFTDataPackage::MessageType::PREPARE)
				break;

			if (m_state->requestLog.find({ *data->puiViewNumber, *data->puiSequenceNumber }) == m_state->requestLog.end())
				break;
			else
			{
				if (m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest == nullptr)
					break;

				if (m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->pClientRequest == nullptr)
					break;
			}

			// Высчитывается f, как [(N - 1) / 3],  где [] – округление в большую сторону
			unsigned int f = (((m_vvbPublicKeys.size() - 1) - 1) / 3) + 1;

			// Если кол-во полученных prepare ещё не 2f, оно наращивается и обработка пакета прекращается
			if (++m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].uiPreparesRecieved < (2 * f))
			{
				m_pLogger->LogMessage("Got " + std::to_string(m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].uiPreparesRecieved) + " preprepares\n");
				break;
			}
				
			m_pLogger->LogMessage("Got enough preprepares\n");

			// На этом моменте предикат prepared выполнен

			m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].bPrepared = true;

			// Создаётся пакет commit (утечка памяти пока не устранена, т.к. непонятно, 
			// в какой момент все потоки завершат обработаку данного пакета, ятобы можно было чистить память)
			SBFTDataPackage* commitPackage{ new SBFTDataPackage(m_id, UINT_MAX, SBFTDataPackage::MessageType::COMMIT) };
			commitPackage->puiSequenceNumber = new unsigned int{ *data->puiSequenceNumber };
			commitPackage->puiViewNumber = new unsigned int{ *data->puiViewNumber };
			commitPackage->vbMessageDigest = m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->pClientRequest->GetDataHash();
			commitPackage->puiNodeId = new unsigned int{ m_id };
			commitPackage->vbSign = m_pSignatureManager->Sign(commitPackage->GetRawData(), m_vbPrivateKey);

			m_pNetwork->PostData(commitPackage);

			m_state->state = State::COMMIT;

			// Сообщение в лог ---------------------------------------------------------------------------------------------------
			m_pLogger->LogMessage("Switched to commit");

			break;
		}

		case State::COMMIT:
		{
			if (data->type != SBFTDataPackage::MessageType::COMMIT)
				break;

			if (m_state->requestLog.find({ *data->puiViewNumber, *data->puiSequenceNumber }) == m_state->requestLog.end())
				break;
			else
			{
				if (m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].prepareRequest->vbMessageDigest != data->vbMessageDigest)
					break;
			}

			if (!m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].bPrepared)
				break;

			unsigned int f = (((m_vvbPublicKeys.size() - 1) - 1) / 3) + 1;

			// Если кол-во полученных prepare ещё не f + 1, оно наращивается и обработка пакета прекращается
			if (++m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].uiCommitsRecieved < (f + 1))
			{
				m_pLogger->LogMessage("Got " + std::to_string(m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].uiCommitsRecieved) + " preprepares\n");
				break;
			}

			// На этом моменте предикат committed-local выполнен

			if (m_state->state != State::COMMIT)
				break;

			m_state->state = State::PRE_PREPARE;
			bRequestRecieved = false;

			// Сообщение в лог ---------------------------------------------------------------------------------------------------
			m_pLogger->LogMessage("Switched from commit to preprepare");

			m_chain->AddBlock(*m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->pClientRequest->pBlock);

			unsigned int requestSenderId = m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->pClientRequest->uiSender;

			// Создаётся пакет reply (утечка памяти пока не устранена, т.к. непонятно, 
			// в какой момент все потоки завершат обработаку данного пакета, ятобы можно было чистить память)
			SBFTDataPackage* replyPackage{ new SBFTDataPackage(m_id, requestSenderId, SBFTDataPackage::MessageType::REPLY) };
			replyPackage->puiViewNumber = new unsigned int{ *data->puiViewNumber };
			replyPackage->pTimestamp = new std::chrono::high_resolution_clock::time_point{ *m_state->requestLog[{ *data->puiViewNumber,* data->puiSequenceNumber }].preprepareRequest->pClientRequest->pTimestamp };
			replyPackage->pBlock = new Block{ *m_state->requestLog[{ *data->puiViewNumber,* data->puiSequenceNumber }].preprepareRequest->pClientRequest->pBlock };
			replyPackage->puiNodeId = new unsigned int{ m_id };
			replyPackage->vbSign = m_pSignatureManager->Sign(replyPackage->GetRawData(), m_vbPrivateKey);

			m_pNetwork->PostData(replyPackage);

			break;
		}

		default:
			break;
		}
	}
}


void SBFTNode::AddNewBlock()
{
	std::vector<SBFTDataPackage*> dataChunksForMining(10);

	m_poolMutex->lock();

	// Проверка на случай, если закрепление блока уже было начато другим потоком
	if (m_transactionPool->size() < 10)
	{
		m_poolMutex->unlock();
		return;
	}
	// Из пула копируются первые 10 пакетов
	std::copy(m_transactionPool->begin(), std::next(m_transactionPool->begin(), 10), dataChunksForMining.begin());
	// Из пула удаляются скопированные 10 пакетов
	m_transactionPool->erase(m_transactionPool->begin(), std::next(m_transactionPool->begin(), 10));
	m_poolMutex->unlock();

	// Все транзакции объединяются в вектор
	std::vector<std::string> messagesToBlock;
	for (SBFTDataPackage* dataChunk : dataChunksForMining)
	{
		messagesToBlock.push_back(dataChunk->message);
	}

	// Создаётся новый блок
	Block* newBlock{ new Block(m_chain->GetLastBlock().GetIndex() + 1, messagesToBlock) };

	// Здесь начинается протокол консенсуса

	// Создаётся пакет request
	SBFTDataPackage* requestPackage{ new SBFTDataPackage(
		m_id, 
		m_state->uiCurrentViewNumber % m_vvbPublicKeys.size(), // В роли меры количества участников выступает количество доступных открытых ключей
		SBFTDataPackage::MessageType::REQUEST) };

	requestPackage->pBlock		= newBlock;
	requestPackage->pTimestamp	= new std::chrono::high_resolution_clock::time_point{ std::chrono::high_resolution_clock::now() };
	requestPackage->puiNodeId	= new unsigned int{ m_id };
	requestPackage->vbSign		= m_pSignatureManager->Sign(requestPackage->GetRawData(), m_vbPrivateKey);

	// request публикуется
	m_pNetwork->PostData(requestPackage);

	// Устанавливается флаг, что даннный узел ждёт решения по закреплению пакета
	m_state->bIsWaitingForReply = { true, newBlock };

	//m_pAddingBlockInProcessMutex->unlock();
}
