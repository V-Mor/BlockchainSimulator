#pragma once

#include "Node.h"
#include "PBFTDataPackage.h"
#include "NodeLogger.h"


//
// Represents the node performing PBFT protocol
//
// All logic is based on the article Miguel Castro and Barbara Liskov – Practical Byzantine Fault Tolerance (1999)
// So far, only the part "4.2 Normal-Case Operation" has been implemented 
//
class PBFTNode : public Node
{
public:
	typedef std::shared_ptr<PBFTNode> Ptr;

	PBFTNode(unsigned int id,
		NetworkManager1::Ptr pNetwork,
		DigitalSignatureManager::Ptr pSignatureManager,
		ByteVector vbPrivateKey,
		std::vector<ByteVector>& const vbPublicKeys);
	~PBFTNode();

	void operator()();
	void PrintSavedBlocks() const;
	size_t GetStoredBlocksNumber() const;
	size_t GetPoolSize() const;
	void Stop();
	void ListenMessages();
	void AddNewBlock();
	void WaitForReply(PBFTDataPackage* requestPackage);
	size_t GetTotalResendingsNumber();
	double CountLostTransactions();
	size_t GetQueueLength();
	size_t GetPoolSize();

private:
	// State (according to the article)
	enum class State { NONE, PRE_PREPARE, PREPARE, COMMIT, REPLY };

	//
	// Represents a request log entry
	//
	struct RequestLogRecording
	{
		// The number of PREPARE messages recieved for the current request
		unsigned int		uiPreparesRecieved;

		// The number of COMMIT messages recieved for the current request
		unsigned int		uiCommitsRecieved;

		// The first PREPREPARE message recieved for the current request
		PBFTDataPackage*	preprepareRequest;

		// The first PREPARE message recieved for the current request
		PBFTDataPackage*	prepareRequest;

		// Flag showing that the "prepared" predicate is true for this request
		bool				bPrepared;
	};

	//
	// The structure that represents a node state according to the PBFT protocol
	// Includes a state of the state machine, current view number (is not changed in current implementation),
	// current request number (all the rest is described later)
	//
	struct PBFTState
	{
		State					state;
		unsigned int			uiCurrentViewNumber;
		unsigned int			uiCurrentRequestNumber;
		unsigned int			uiRepliesRecieved;

		// Shows if this node waits for consensus for the block sent
		std::pair<bool, Block*> bIsWaitingForReply;

		std::chrono::high_resolution_clock::time_point	requestTimestamp;
		std::vector<std::chrono::milliseconds>	waitingTimes;

		// Key is the pair made of view number and request number and value is the log of this request
		std::map<std::pair<unsigned int, unsigned int>, RequestLogRecording> requestLog;
	};

	std::shared_ptr<std::vector<PBFTDataPackage*>>	m_transactionPool;
	std::shared_ptr<PBFTState>						m_state;
	std::shared_ptr<std::mutex>						m_pAddingBlockInProcessMutex;
	std::shared_ptr<NodeLogger>						m_pLogger;

	std::shared_ptr<std::mutex>						m_pSenderStoppedMutex;
	std::shared_ptr<std::mutex>						m_pListenerStoppedMutex;
	std::shared_ptr<std::mutex>						m_pRequestResenderStoppedMutex;

	std::shared_ptr<size_t>							m_puiTotalResendingsNumber;

	std::shared_ptr<std::vector<std::string>>		m_allUniqueMessages;
};

