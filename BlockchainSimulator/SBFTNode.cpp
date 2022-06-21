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
	m_state(new SBFTState { State::PRE_PREPARE, 0, 0}), // ��������� ���������������� ��� PREPREPARE, ����� view 0, ����� �������� ������� 0
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
	// ����� �� ���������� Node, ������ ������ SimpleDataPackage ������������ 
	// SBFTDataPackage � ����� COMMON_MESSAGE, ��� ��������, ��� �� �������� ������ ��������� ���������

	std::thread listenThread(&SBFTNode::ListenMessages, this);

	std::mt19937::result_type seed = time(0);
	// (��� ���� �� ���������� ��� ������������� ����������)
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


// ��������� ���� ���� ����������� ������
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
	// ��� �������� �������� (��� ���� �� ���������� ��� ������������� ����������)
	std::mt19937::result_type seed = time(0);
	auto getRandomInt = std::bind(std::uniform_int_distribution<int>(12, 25), std::mt19937(seed));

	while (!m_bStopped->load())
	{
		// �������� �������� (�����������������, ���� �����)

		//std::this_thread::sleep_for(std::chrono::milliseconds(getRandomInt()));

		// ��������� ������, ���������� ����� ���� (��������� �����, ���� ������ ���� ���)
		SBFTDataPackage* data = dynamic_cast<SBFTDataPackage*>(m_pNetwork->GetData(m_id));

		m_pLogger->LogMessage("Got message of type " + std::to_string(static_cast<int>(data->type)));

		// ���� ������ �� �������������� �� ��������� ����� ����, �� ���� ����� � ����, ����� �������������
		if (!(data->uiDestination == m_id ||
			data->uiDestination == UINT_MAX))
			continue;

		// ���� ������� �������, ����� �������������
		if (!m_pSignatureManager->Verify(data->GetRawData(), data->vbSign, m_vvbPublicKeys[data->uiSender]))
			continue;

		// ���� ����� �������� ���������������� ��������� (������ ����������)
		if (data->type == SBFTDataPackage::MessageType::COMMON_MESSAGE)
		{
			std::lock_guard<std::mutex> chainLock(*m_poolMutex);
			// ��������� ������������ � ���
			m_transactionPool->push_back(data);

			// ���� ��������� � ���� 10 ��� ������
			if (m_transactionPool->size() >= 10)
			{
				//if (m_pAddingBlockInProcessMutex->try_lock())
				{
					// � ����� ������ ���������� �������� ����� � ��� �����������
					std::thread t(&SBFTNode::AddNewBlock, this);
					t.detach();
				}
			}

			continue;
		}

		// ���� � ������ �� ������� ���������, ��������� ������������ � ����������� �� ���������
		switch (m_state->state)
		{
		case State::PRE_PREPARE:
		{
			// ���� ������ ������ �� �������
			if (data->type == SBFTDataPackage::MessageType::REQUEST)
			{
				// ���� �������� REQUEST, �� ��� ���� �� Primary, ����� �������������
				if (!(m_state->uiCurrentViewNumber % m_vvbPublicKeys.size() == m_id))
					break;
				else
				{
					// ��������������� ����, ��� ������ �������. ���� ������ ��� ��� �������, ����� �������������
					if (!bRequestRecieved)
						bRequestRecieved = true;
					else
						break;

					// �������� ����� preprepare (������ ������ ���� �� ���������, �.�. ���������, 
					// � ����� ������ ��� ������ �������� ���������� ������� ������, ����� ����� ���� ������� ������)
					//
					// ����� � ����� � ������ ����������� ������ ������ ����. ��������������, ��� ����������� ����� 
					// ������ ������ ������ ����, ������������ �� ���� ������
					SBFTDataPackage* prepreparePackage{ new SBFTDataPackage(m_id, UINT_MAX, SBFTDataPackage::MessageType::PREPREPARE) };
					prepreparePackage->pClientRequest = data;
					prepreparePackage->puiSequenceNumber = new unsigned int{ m_state->uiCurrentRequestNumber++ };
					prepreparePackage->puiViewNumber = new unsigned int{ m_state->uiCurrentViewNumber };
					prepreparePackage->vbMessageDigest = data->GetDataHash();
					// ����� �������������
					prepreparePackage->vbSign = m_pSignatureManager->Sign(prepreparePackage->GetRawData(), m_vbPrivateKey);

					// ����� ����������� ����
					m_pNetwork->PostData(prepreparePackage);

					// ����������� ����� ���������, �� � ��� � ��
					m_state->state = State::PRE_PREPARE;

					// ��������� � ��� ---------------------------------------------------------------------------------------------------
					m_pLogger->LogMessage("Switched to preprepare");

					break;
				}
			}

			// ���� ��� ���� � ������, ������������ ���� � ��� �������� ����� �� ��������� �����������
			else if (data->type == SBFTDataPackage::MessageType::REPLY)
			{
				// ���� ���� �� ���� �����������, � ��� ��� ������, ����� �������������
				if (!m_state->bIsWaitingForReply.first)
					break;

				// ���� �������� ��� �����, ������� ������������
				if (data->pBlock->GetHash() == m_state->bIsWaitingForReply.second->GetHash())
					; // ����� ������ ���� �����, ����������, ��� �� ������ � ���� ��������
			}

			else
			{
				// ���� ��������� �� PREPREPARE, ����� �������������
				if (data->type != SBFTDataPackage::MessageType::PREPREPARE)
					break;

				// ���� ��� �������, ����� �������������
				if (data->pClientRequest->GetDataHash() != data->vbMessageDigest)
					break;

				// ���� View �� ���, ����� �������������
				if (*data->puiViewNumber != m_state->uiCurrentViewNumber)
					break;

				// ���� � ���� ������ ������� ��� ���, �����������
				if (m_state->requestLog.find({ *data->puiViewNumber, *data->puiSequenceNumber }) == m_state->requestLog.end())
					m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }] = { 0, 0, data, nullptr, false };
				else
					// ���� �� � ���� �� �� ���� ����� ������, �� ���������� ����� �� ��������� �� ����������, �� �������������
					if (m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->vbMessageDigest != data->vbMessageDigest)
						break;

				// ���� ���������� ����� ������� �� ������������� ������������, ����� �������������
				if (!(0 <= *data->puiSequenceNumber <= UINT_MAX))
					break;

				// �������� ����� prepare (������ ������ ���� �� ���������, �.�. ���������, 
				// � ����� ������ ��� ������ �������� ���������� ������� ������, ����� ����� ���� ������� ������)
				SBFTDataPackage* preparePackage{ new SBFTDataPackage(m_id, UINT_MAX, SBFTDataPackage::MessageType::PREPARE) };
				preparePackage->puiSequenceNumber = new unsigned int{ *data->puiSequenceNumber };
				preparePackage->puiViewNumber = new unsigned int{ *data->puiViewNumber };
				preparePackage->vbMessageDigest = data->vbMessageDigest;
				preparePackage->puiNodeId = new unsigned int{ m_id };
				preparePackage->vbSign = m_pSignatureManager->Sign(preparePackage->GetRawData(), m_vbPrivateKey);

				m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].prepareRequest = preparePackage;

				m_pNetwork->PostData(preparePackage);

				m_state->state = State::PREPARE;

				// ��������� � ��� ---------------------------------------------------------------------------------------------------
				m_pLogger->LogMessage("Switched to prepare");

				break;
			}
		}

		case State::PREPARE:
		{
			// �������� ������ �� ����������, ����� ���������� ����������� ���������
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

			// ������������� f, ��� [(N - 1) / 3],  ��� [] � ���������� � ������� �������
			unsigned int f = (((m_vvbPublicKeys.size() - 1) - 1) / 3) + 1;

			// ���� ���-�� ���������� prepare ��� �� 2f, ��� ������������ � ��������� ������ ������������
			if (++m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].uiPreparesRecieved < (2 * f))
			{
				m_pLogger->LogMessage("Got " + std::to_string(m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].uiPreparesRecieved) + " preprepares\n");
				break;
			}
				
			m_pLogger->LogMessage("Got enough preprepares\n");

			// �� ���� ������� �������� prepared ��������

			m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].bPrepared = true;

			// �������� ����� commit (������ ������ ���� �� ���������, �.�. ���������, 
			// � ����� ������ ��� ������ �������� ���������� ������� ������, ����� ����� ���� ������� ������)
			SBFTDataPackage* commitPackage{ new SBFTDataPackage(m_id, UINT_MAX, SBFTDataPackage::MessageType::COMMIT) };
			commitPackage->puiSequenceNumber = new unsigned int{ *data->puiSequenceNumber };
			commitPackage->puiViewNumber = new unsigned int{ *data->puiViewNumber };
			commitPackage->vbMessageDigest = m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->pClientRequest->GetDataHash();
			commitPackage->puiNodeId = new unsigned int{ m_id };
			commitPackage->vbSign = m_pSignatureManager->Sign(commitPackage->GetRawData(), m_vbPrivateKey);

			m_pNetwork->PostData(commitPackage);

			m_state->state = State::COMMIT;

			// ��������� � ��� ---------------------------------------------------------------------------------------------------
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

			// ���� ���-�� ���������� prepare ��� �� f + 1, ��� ������������ � ��������� ������ ������������
			if (++m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].uiCommitsRecieved < (f + 1))
			{
				m_pLogger->LogMessage("Got " + std::to_string(m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].uiCommitsRecieved) + " preprepares\n");
				break;
			}

			// �� ���� ������� �������� committed-local ��������

			if (m_state->state != State::COMMIT)
				break;

			m_state->state = State::PRE_PREPARE;
			bRequestRecieved = false;

			// ��������� � ��� ---------------------------------------------------------------------------------------------------
			m_pLogger->LogMessage("Switched from commit to preprepare");

			m_chain->AddBlock(*m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->pClientRequest->pBlock);

			unsigned int requestSenderId = m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->pClientRequest->uiSender;

			// �������� ����� reply (������ ������ ���� �� ���������, �.�. ���������, 
			// � ����� ������ ��� ������ �������� ���������� ������� ������, ����� ����� ���� ������� ������)
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

	// �������� �� ������, ���� ����������� ����� ��� ���� ������ ������ �������
	if (m_transactionPool->size() < 10)
	{
		m_poolMutex->unlock();
		return;
	}
	// �� ���� ���������� ������ 10 �������
	std::copy(m_transactionPool->begin(), std::next(m_transactionPool->begin(), 10), dataChunksForMining.begin());
	// �� ���� ��������� ������������� 10 �������
	m_transactionPool->erase(m_transactionPool->begin(), std::next(m_transactionPool->begin(), 10));
	m_poolMutex->unlock();

	// ��� ���������� ������������ � ������
	std::vector<std::string> messagesToBlock;
	for (SBFTDataPackage* dataChunk : dataChunksForMining)
	{
		messagesToBlock.push_back(dataChunk->message);
	}

	// �������� ����� ����
	Block* newBlock{ new Block(m_chain->GetLastBlock().GetIndex() + 1, messagesToBlock) };

	// ����� ���������� �������� ����������

	// �������� ����� request
	SBFTDataPackage* requestPackage{ new SBFTDataPackage(
		m_id, 
		m_state->uiCurrentViewNumber % m_vvbPublicKeys.size(), // � ���� ���� ���������� ���������� ��������� ���������� ��������� �������� ������
		SBFTDataPackage::MessageType::REQUEST) };

	requestPackage->pBlock		= newBlock;
	requestPackage->pTimestamp	= new std::chrono::high_resolution_clock::time_point{ std::chrono::high_resolution_clock::now() };
	requestPackage->puiNodeId	= new unsigned int{ m_id };
	requestPackage->vbSign		= m_pSignatureManager->Sign(requestPackage->GetRawData(), m_vbPrivateKey);

	// request �����������
	m_pNetwork->PostData(requestPackage);

	// ��������������� ����, ��� ������� ���� ��� ������� �� ����������� ������
	m_state->bIsWaitingForReply = { true, newBlock };

	//m_pAddingBlockInProcessMutex->unlock();
}
