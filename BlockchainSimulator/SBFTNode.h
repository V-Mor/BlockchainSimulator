#pragma once

#include "Node.h"
#include "SBFTDataPackage.h"
#include "NodeLogger.h"


//
// ѕредставл€ет узел сети, работающий по протоколу SBFT
//
// ¬с€ логика реализована согласно статье Miguel Castro and Barbara Liskov Ц Practical Byzantine Fault Tolerance (1999)
// Ќа данный момент реализована только часть 4.2 Normal-Case Operation
//
class SBFTNode : public Node
{
public:
	typedef std::shared_ptr<SBFTNode> Ptr;

	SBFTNode(unsigned int id,
		NetworkManager1::Ptr pNetwork,
		DigitalSignatureManager::Ptr pSignatureManager,
		ByteVector vbPrivateKey,
		std::vector<ByteVector>& const vbPublicKeys);
	~SBFTNode();

	void operator()();
	void PrintTransactionsFromPool() const;
	void Stop();
	void ListenMessages();
	void AddNewBlock();

private:
	// —осто€ние (по статье)
	enum class State {	REQUEST,
						PRE_PREPARE, 
						SIGN_SHARE,
						FULL_COMMIT_PROOF,
						SIGN_STATE,
						FULL_EXECUTE_PROOF,
						EXECUTE_ACK };

	// –оль в протоколе
	enum class Role { CLIENT, REPLICA, PRIMARY, C_COLLECTOR, E_COLLECTOR };

	//
	// ѕредставл€ет запись в логе запросов
	//
	struct RequestLogRecording
	{
		//  оличество прин€тых сообщений SIGN-SHARE дл€ данного запроса
		unsigned int		uiSignSharesRecieved;

		//  оличество прин€тых сообщений SIGN-STATE дл€ данного запроса
		unsigned int		uiSignStatesRecieved;

		// ѕервое полученное сообщение PREPREPARE дл€ данного запроса
		SBFTDataPackage*	preprepareRequest;

		// ѕервое полученное сообщение PREPARE дл€ данного запроса
		SBFTDataPackage*	prepareRequest;

		// ‘лаг, показывающий, что предикат prepared выполнен дл€ данного запроса
		bool				bPrepared;
	};

	//
	// —труктура, представл€юща€ состо€ние узла, согласно протоколу SBFT
	// —одержит состо€ние конечного автомата, текущий номер view (не мен€етс€ в текущей реализации),
	// текущий номер запроса (остальное описано далее)
	//
	struct SBFTState
	{
		State					state;
		unsigned int			uiCurrentViewNumber;
		unsigned int			uiCurrentRequestNumber;

		// ѕредставл€ет запись лога: view и пор€дковый номер запроса в виде ключа и лог данного запроса в виде значени€
		std::map<std::pair<unsigned int, unsigned int>, RequestLogRecording> requestLog;
	};

	std::shared_ptr<std::vector<SBFTDataPackage*>>	m_transactionPool;
	std::shared_ptr<SBFTState>						m_state;
	std::shared_ptr<std::mutex>						m_pAddingBlockInProcessMutex;
	std::shared_ptr<NodeLogger>						m_pLogger;
	Role											m_role;
};

