#include "StdAfx.h"

#include "PBFTNode.h"
#include <random>
#include <iomanip>
#include <fstream>


extern double INTERVAL_AVERAGE;
extern int PERIOD_AVERAGE;

extern unsigned int BLOCK_SIZE;
extern unsigned int RESENDING_TIMEOUT;
unsigned int const MESSAGE_SIZE = 100;


PBFTNode::PBFTNode(unsigned int id,
	NetworkManager1::Ptr pNetwork,
	DigitalSignatureManager::Ptr pSignatureManager,
	ByteVector vbPrivateKey,
	std::vector<ByteVector>& const vbPublicKeys) :
	Node(id, pNetwork, pSignatureManager, vbPrivateKey, vbPublicKeys),
	m_state(new PBFTState{ State::PRE_PREPARE, 0, 0, 0 }), // State is initialized like PREPREPARE, view number 0, current request number 0, responses recieved 0
	m_pLogger(new NodeLogger(id)),
	m_puiTotalResendingsNumber(new size_t(0))
{
	m_transactionPool = std::shared_ptr<std::vector<PBFTDataPackage*>>(new std::vector<PBFTDataPackage*>);
	m_pAddingBlockInProcessMutex.reset(new std::mutex);
	m_pSenderStoppedMutex.reset(new std::mutex);
	m_pListenerStoppedMutex.reset(new std::mutex);
	m_pRequestResenderStoppedMutex.reset(new std::mutex);
	m_allUniqueMessages.reset(new std::vector<std::string>);
}


PBFTNode::~PBFTNode()
{
	m_pRequestResenderStoppedMutex->lock();
	m_pLogger->LogMessage(std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + " Node with id " + std::to_string(m_id) + " destructing");
	m_pRequestResenderStoppedMutex->unlock();
}


void PBFTNode::operator()()
{
	// Everithing here is like in Node, but instead of SimpleDataPackage 
	// PBFTDataPackage with type COMMON_MESSAGE is sent, it means that it contains the text message only

	std::thread listenThread(&PBFTNode::ListenMessages, this);

	std::mt19937::result_type seed = time(0);
	// (that is one of the variable during the experiments parameters)
	auto getRandomInt = std::bind(std::uniform_int_distribution(4, 8), std::mt19937(seed));

	// Sender started
	m_pSenderStoppedMutex->lock();

	while (!m_bStopped->load())
	{
		std::this_thread::sleep_for(std::chrono::seconds(std::max(0, static_cast<int>(getRandomInt()))));

		PBFTDataPackage* dataToSend{ new PBFTDataPackage(m_id, UINT_MAX, PBFTDataPackage::MessageType::COMMON_MESSAGE) };
		//dataToSend->message = "Hello from node with ID " + std::to_string(m_id);

		// Message of length 100 bytes
		dataToSend->message = std::to_string(m_id);
		for (int i = 0; i < MESSAGE_SIZE; ++i)
			dataToSend->message += "1234568790";

		std::string uniqueMessagePart = std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		dataToSend->message += uniqueMessagePart;
		m_allUniqueMessages->push_back(uniqueMessagePart);

		dataToSend->vbSign = m_pSignatureManager->Sign(dataToSend->GetRawData(), m_vbPrivateKey);

		m_pNetwork->PostData(dataToSend);
	}

	// Sender is stopped
	m_pSenderStoppedMutex->unlock();

	listenThread.join();
}


// Prints all the commited blocks' hashes
void PBFTNode::PrintSavedBlocks() const
{
	std::lock_guard<std::mutex> chainLock(*m_poolMutex);

	std::cout << std::dec << "Node with id " << m_id << ":\n";

	int i = 0;
	for (Block& block : m_chain->GetChain())
	{
		std::cout << "Block #" << std::dec << i << " with hash: ";
		for (CryptoPP::byte& hashByte : block.GetHash())
			std::cout << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(hashByte);
		std::cout << std::endl;
		++i;
	}

	std::ofstream log("logfile.log", std::ios_base::app);
	for (std::chrono::milliseconds& timeDelta : m_state->waitingTimes)
		log << std::dec << timeDelta.count() << "\t";
	log.close();

	std::cout << std::endl;
}


size_t PBFTNode::GetPoolSize() const
{
	return m_transactionPool->size();
}


size_t PBFTNode::GetStoredBlocksNumber() const
{
	return m_chain->GetChain().size();
}


void PBFTNode::Stop()
{
	m_bStopped->store(true);

	// While Sender and Listener is not stopped, Stop doesn't return
	std::lock(*m_pListenerStoppedMutex, *m_pSenderStoppedMutex);

	/*m_pLogger->LogMessage(std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + " Node with id " + std::to_string(m_id) + " trying lock in stop");
	m_pRequestResenderStoppedMutex->lock();
	m_pLogger->LogMessage(std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + " Node with id " + std::to_string(m_id) + " locked in stop");*/

	m_pListenerStoppedMutex->unlock();
	m_pSenderStoppedMutex->unlock();
	/*m_pRequestResenderStoppedMutex->unlock();*/
}


void PBFTNode::ListenMessages()
{
	// To simulate a delay (this is one of the parameters that can be changed for experiments)
	std::mt19937::result_type seed = time(0);
	auto getRandomDouble = std::bind(std::normal_distribution(2.0, 2.0 / (2*sqrt(3.0))), std::mt19937(seed));

	// Flag that means that request is recieved
	// If this flag is set, new requests are not handled. preprepare message for the current request is waited instead
	bool bRequestRecieved{ false };

	// Listener is started
	m_pListenerStoppedMutex->lock();

	while (!m_bStopped->load())
	{
		// Recieving of the data sent for this node (blocks the current thread if no data are presented)
		PBFTDataPackage* data = dynamic_cast<PBFTDataPackage*>(m_pNetwork->GetData(m_id));

		// Delay simulation (should be uncommented if it is needed)

		//std::this_thread::sleep_for(std::chrono::milliseconds(std::max(0, static_cast<int>(getRandomDouble()))));
		if (m_bStopped->load())
			break;

		//m_pLogger->LogMessage("Got message of type " + std::to_string(static_cast<int>(data->type)));

		// If the data is not intended for this particular node or all nodes in the network, the package is dropped
		if (!(data->uiDestination == m_id ||
			data->uiDestination == UINT_MAX))
			continue;

		try
		{
			// If the signature is invalid, the package is dropped
			if (!m_pSignatureManager->Verify(data->GetRawData(), data->vbSign, m_vvbPublicKeys[data->uiSender]))
				continue;
		}
		catch (...)
		{
			break;
		}

		// If the package contains usual text message (like a common transaction)
		if (data->type == PBFTDataPackage::MessageType::COMMON_MESSAGE)
		{
			std::lock_guard<std::mutex> chainLock(*m_poolMutex);
			// The message is added to the pool
			m_transactionPool->push_back(data);

			// If there is more than BLOCK_SIZE messages in the pool
			if (m_transactionPool->size() >= BLOCK_SIZE)
			{
				if (!m_bStopped->load())
				{
					// Block creation is started in a new thread
					std::thread t(&PBFTNode::AddNewBlock, this);
					t.detach();
				}
				else
					break;
			}

			continue;
		}

		// If the package doesn't contain a common message, it is handled according to the current state
		switch (m_state->state)
		{
		case State::PRE_PREPARE:
		{
			// Client's request recieved
			if (data->type == PBFTDataPackage::MessageType::REQUEST)
			{
				// If REQUEST is recieved but this node is not Primary, the package is dropped
				if (!(m_state->uiCurrentViewNumber % m_vvbPublicKeys.size() == m_id))
					break;
				else
				{
					// The RequestRecieved flag is set. If the request has already been received, the package is dropped.
					if (!bRequestRecieved)
						bRequestRecieved = true;
					else
						break;

					// preprepare package is created
					//
					// Olnly the needed fields are filled
					PBFTDataPackage* prepreparePackage{ new PBFTDataPackage(m_id, UINT_MAX, PBFTDataPackage::MessageType::PREPREPARE) };
					prepreparePackage->pClientRequest.reset(data);
					prepreparePackage->puiSequenceNumber.reset( new unsigned int{ m_state->uiCurrentRequestNumber++ });
					prepreparePackage->puiViewNumber.reset(new unsigned int{ m_state->uiCurrentViewNumber });
					prepreparePackage->vbMessageDigest = data->GetDataHash();
					// The package is signed
					prepreparePackage->vbSign = m_pSignatureManager->Sign(prepreparePackage->GetRawData(), m_vbPrivateKey);

					// The package is sent to all other nodes
					m_pNetwork->PostData(prepreparePackage);

					// Useless state change
					m_state->state = State::PRE_PREPARE;

					//m_pLogger->LogMessage("Switched to preprepare");

					break;
				}
			}

			// If this node is a client commiting the block and it recieve the response about successful commit
			else if (data->type == PBFTDataPackage::MessageType::REPLY)
			{
				// If this node didn't wait for the commit message but recieved it, the package is dropped
				if (!m_state->bIsWaitingForReply.first)
					break;

				// If the right package is commited
				if (data->pBlock->GetHash() == m_state->bIsWaitingForReply.second->GetHash())
				{
					unsigned int f = (((m_vvbPublicKeys.size() - 1) - 1) / 3) + 1;
					if (++m_state->uiRepliesRecieved < (f + 1))
						break;

					m_state->waitingTimes.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()) - std::chrono::duration_cast<std::chrono::milliseconds>(m_state->requestTimestamp.time_since_epoch()));
					m_chain->AddBlock(*m_state->bIsWaitingForReply.second);
					m_state->bIsWaitingForReply.first = false;
					m_state->uiRepliesRecieved = 0;
				}
			}

			else
			{
				// If the message is not PREPREPARE, the package is dropped
				if (data->type != PBFTDataPackage::MessageType::PREPREPARE)
					break;

				// If the hash value is invalid, the package is dropped
				if (data->pClientRequest->GetDataHash() != data->vbMessageDigest)
					break;

				// If the View is not correct, the package is dropped
				if (*data->puiViewNumber != m_state->uiCurrentViewNumber)
					break;

				// If there is no such request in log, it is added
				if (m_state->requestLog.find({ *data->puiViewNumber, *data->puiSequenceNumber }) == m_state->requestLog.end())
					m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }] = { 0, 0, data, nullptr, false };
				else
					// If there is such request in the log but its parameters is not the same, the package is dropped
					if (m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->vbMessageDigest != data->vbMessageDigest)
						break;

				// If the request number doesn't fit the restrictions, the package is dropped
				if (!(0 <= *data->puiSequenceNumber <= UINT_MAX))
					break;

				// prepare package is created
				PBFTDataPackage* preparePackage{ new PBFTDataPackage(m_id, UINT_MAX, PBFTDataPackage::MessageType::PREPARE) };
				preparePackage->puiSequenceNumber.reset(new unsigned int{ *data->puiSequenceNumber });
				preparePackage->puiViewNumber.reset( new unsigned int{ *data->puiViewNumber });
				preparePackage->vbMessageDigest = data->vbMessageDigest;
				preparePackage->puiNodeId.reset( new unsigned int{ m_id });
				preparePackage->vbSign = m_pSignatureManager->Sign(preparePackage->GetRawData(), m_vbPrivateKey);

				m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].prepareRequest = preparePackage;

				m_pNetwork->PostData(preparePackage);

				m_state->state = State::PREPARE;

				//m_pLogger->LogMessage("Switched to prepare");

				break;
			}
		}

		case State::PREPARE:
		{
			// Checking the package for validity, almost the same as the previous state
			if (data->type != PBFTDataPackage::MessageType::PREPARE)
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

			// f is computed as [(N - 1) / 3],  where [] is rounding up
			unsigned int f = (((m_vvbPublicKeys.size() - 1) - 1) / 3) + 1;

			// If the number of prepare recieved is still not 2f, it inis increased and the package handling is stopped
			if (++m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].uiPreparesRecieved < (2 * f))
			{
				//m_pLogger->LogMessage("Got " + std::to_string(m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].uiPreparesRecieved) + " preprepares\n");
				break;
			}
				
			//m_pLogger->LogMessage("Got enough preprepares\n");

			// prepared predicate is true on this line

			m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].bPrepared = true;

			// Commit package is created
			PBFTDataPackage* commitPackage{ new PBFTDataPackage(m_id, UINT_MAX, PBFTDataPackage::MessageType::COMMIT) };
			commitPackage->puiSequenceNumber.reset( new unsigned int{ *data->puiSequenceNumber });
			commitPackage->puiViewNumber.reset( new unsigned int{ *data->puiViewNumber });
			commitPackage->vbMessageDigest = m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->pClientRequest->GetDataHash();
			commitPackage->puiNodeId.reset( new unsigned int{ m_id });
			commitPackage->vbSign = m_pSignatureManager->Sign(commitPackage->GetRawData(), m_vbPrivateKey);

			m_pNetwork->PostData(commitPackage);

			m_state->state = State::COMMIT;

			//m_pLogger->LogMessage("Switched to commit");

			break;
		}

		case State::COMMIT:
		{
			if (data->type != PBFTDataPackage::MessageType::COMMIT)
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

			// If the number of prepare recieved is still not f + 1, is is increased and package handling is stopped
			if (++m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].uiCommitsRecieved < (f + 1))
			{
				//m_pLogger->LogMessage("Got " + std::to_string(m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].uiCommitsRecieved) + " preprepares\n");
				break;
			}

			// committed-local predicate is true on this line

			if (m_state->state != State::COMMIT)
				break;

			m_state->state = State::PRE_PREPARE;
			bRequestRecieved = false;

			//m_pLogger->LogMessage("Switched from commit to preprepare");

			if (m_chain->GetLastBlock().GetHash() == m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->pClientRequest->pBlock->GetHash())
				break;

			if (!(m_state->bIsWaitingForReply.first && m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->pClientRequest->pBlock->GetHash() == m_state->bIsWaitingForReply.second->GetHash()))
			{
				m_chain->AddBlock(*m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->pClientRequest->pBlock);
				m_poolMutex->lock();

				// Drops from the pool all the transactions, which is included to the commited block

				for (std::string commitedTransaction : m_chain->GetLastBlock().GetData())
				{
					std::vector<PBFTDataPackage*> poolcopy = *m_transactionPool;
					for (PBFTDataPackage* packageFromPool : poolcopy)
					{
						if (packageFromPool->message == commitedTransaction)
						{
							//delete packageFromPool;
							m_transactionPool->erase(std::find(m_transactionPool->begin(), m_transactionPool->end(), packageFromPool));
						}
					}
				}
				m_poolMutex->unlock();
			}

			unsigned int requestSenderId = m_state->requestLog[{ *data->puiViewNumber, * data->puiSequenceNumber }].preprepareRequest->pClientRequest->uiSender;

			// reply package is created
			PBFTDataPackage* replyPackage{ new PBFTDataPackage(m_id, requestSenderId, PBFTDataPackage::MessageType::REPLY) };
			replyPackage->puiViewNumber.reset( new unsigned int{ *data->puiViewNumber });
			replyPackage->pTimestamp = new std::chrono::high_resolution_clock::time_point{ *m_state->requestLog[{ *data->puiViewNumber,* data->puiSequenceNumber }].preprepareRequest->pClientRequest->pTimestamp };
			replyPackage->pBlock.reset( new Block{ *m_state->requestLog[{ *data->puiViewNumber,* data->puiSequenceNumber }].preprepareRequest->pClientRequest->pBlock });
			replyPackage->puiNodeId.reset( new unsigned int{ m_id });
			replyPackage->vbSign = m_pSignatureManager->Sign(replyPackage->GetRawData(), m_vbPrivateKey);

			m_pNetwork->PostData(replyPackage);

			break;
		}

		default:
			break;
		}
	}
	// Listener is stopped
	m_pListenerStoppedMutex->unlock();

}


void PBFTNode::AddNewBlock()
{
	unsigned int id = m_id;
	std::vector<PBFTDataPackage*> dataChunksForMining(BLOCK_SIZE);

	// If we are already waiting for confirmation for the next block, we do not create a new one
	if (m_state->bIsWaitingForReply.first)
		return;

	m_poolMutex->lock();

	// Checking if block commit has already been started by another thread
	if (m_transactionPool->size() < BLOCK_SIZE)
	{
		m_poolMutex->unlock();
		return;
	}
	// The first BLOCK_SIZE packages are copied from the pool
	std::copy(m_transactionPool->begin(), std::next(m_transactionPool->begin(), BLOCK_SIZE), dataChunksForMining.begin());
	// BLOCK_SIZE copied packaged are deleted from the pool
	m_transactionPool->erase(m_transactionPool->begin(), std::next(m_transactionPool->begin(), BLOCK_SIZE));
	m_poolMutex->unlock();

	// All transactions are combined into a vector
	std::vector<std::string> messagesToBlock;
	for (PBFTDataPackage* dataChunk : dataChunksForMining)
	{
		messagesToBlock.push_back(dataChunk->message);
	}

	// New block is created
	Block* newBlock{ new Block(m_chain->GetLastBlock().GetIndex() + 1, messagesToBlock) };

	// Consensus protocol starts here

	// request package is created
	PBFTDataPackage* requestPackage{ new PBFTDataPackage(
		m_id, 
		m_state->uiCurrentViewNumber % m_vvbPublicKeys.size(), // The number of available public keys is equal to thu number of the nodes in network
		PBFTDataPackage::MessageType::REQUEST) };

	requestPackage->pBlock.reset( newBlock);
	requestPackage->pTimestamp	= new std::chrono::high_resolution_clock::time_point{ std::chrono::high_resolution_clock::now() };
	requestPackage->puiNodeId.reset( new unsigned int{ m_id });
	requestPackage->vbSign		= m_pSignatureManager->Sign(requestPackage->GetRawData(), m_vbPrivateKey);

	// request is published
	//m_pNetwork->PostData(requestPackage);

	m_state->requestTimestamp = *requestPackage->pTimestamp;

	// The flag is set that this node is waiting for a decision to commit the packet
	m_state->bIsWaitingForReply = { true, newBlock };

	/*std::thread t(&PBFTNode::WaitForReply, this, requestPackage);
	t.detach();*/
	
	bool firstResending = true;

	//m_pLogger->LogMessage(std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) +  " Node with id " + std::to_string(id) + " trying lock in sending");
	m_pRequestResenderStoppedMutex->lock();
	//m_pLogger->LogMessage(std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + " Node with id " + std::to_string(id) + " locked in sending");

	while (!m_bStopped->load() && m_state->bIsWaitingForReply.first && (m_state->bIsWaitingForReply.second->GetHash() == requestPackage->pBlock->GetHash()))
	{
		// If such block has already been committed
		if (m_chain->GetLastBlock().GetHash() == m_state->bIsWaitingForReply.second->GetHash())
		{
			m_state->waitingTimes.push_back(
				std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::high_resolution_clock::now().time_since_epoch()) -
				std::chrono::duration_cast<std::chrono::milliseconds>(m_state->requestTimestamp.time_since_epoch()
					)
			);
			m_state->bIsWaitingForReply.first = false;
			m_state->uiRepliesRecieved = 0;
			break;
		}

		m_pNetwork->PostData(requestPackage);

		if (!firstResending)
			++(*m_puiTotalResendingsNumber);
		else
			firstResending = false;

		std::this_thread::sleep_for(std::chrono::seconds(RESENDING_TIMEOUT));
	}

	//m_pLogger->LogMessage(std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + " Node with id " + std::to_string(id) + " unlocked in sending");
	m_pRequestResenderStoppedMutex->unlock();

	//m_pAddingBlockInProcessMutex->unlock();
}


void PBFTNode::WaitForReply(PBFTDataPackage* requestPackage)
{
	
}


size_t PBFTNode::GetTotalResendingsNumber()
{
	return *m_puiTotalResendingsNumber;
}


double PBFTNode::CountLostTransactions()
{
	double result = 0;

	std::vector<Block> chainCopy = m_chain->GetChain();

	for (std::string messagePart : *m_allUniqueMessages)
	{
		bool found = false;
		for (Block block : chainCopy)
		{
			std::vector<std::string> dataCopy = block.GetData();
			for (std::string dataChunk : dataCopy)
			{
				if (dataChunk.ends_with(messagePart))
				{
					found = true;
					++result;
					break;
				}
			}
			if (found)
				break;
		}
	}

	return 1.0 - result / static_cast<double>(m_allUniqueMessages->size());
}


size_t PBFTNode::GetQueueLength()
{
	return m_pNetwork->GetClientQueueLength(m_id);
}


size_t PBFTNode::GetPoolSize()
{
	return m_transactionPool->size();
}
