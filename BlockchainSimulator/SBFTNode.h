#pragma once

#include "Node.h"
#include "SBFTDataPackage.h"
#include "NodeLogger.h"


//
// ������������ ���� ����, ���������� �� ��������� SBFT
//
// ��� ������ ����������� �������� ������ Miguel Castro and Barbara Liskov � Practical Byzantine Fault Tolerance (1999)
// �� ������ ������ ����������� ������ ����� 4.2 Normal-Case Operation
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
	// ��������� (�� ������)
	enum class State {	REQUEST,
						PRE_PREPARE, 
						SIGN_SHARE,
						FULL_COMMIT_PROOF,
						SIGN_STATE,
						FULL_EXECUTE_PROOF,
						EXECUTE_ACK };

	// ���� � ���������
	enum class Role { CLIENT, REPLICA, PRIMARY, C_COLLECTOR, E_COLLECTOR };

	//
	// ������������ ������ � ���� ��������
	//
	struct RequestLogRecording
	{
		// ���������� �������� ��������� SIGN-SHARE ��� ������� �������
		unsigned int		uiSignSharesRecieved;

		// ���������� �������� ��������� SIGN-STATE ��� ������� �������
		unsigned int		uiSignStatesRecieved;

		// ������ ���������� ��������� PREPREPARE ��� ������� �������
		SBFTDataPackage*	preprepareRequest;

		// ������ ���������� ��������� PREPARE ��� ������� �������
		SBFTDataPackage*	prepareRequest;

		// ����, ������������, ��� �������� prepared �������� ��� ������� �������
		bool				bPrepared;
	};

	//
	// ���������, �������������� ��������� ����, �������� ��������� SBFT
	// �������� ��������� ��������� ��������, ������� ����� view (�� �������� � ������� ����������),
	// ������� ����� ������� (��������� ������� �����)
	//
	struct SBFTState
	{
		State					state;
		unsigned int			uiCurrentViewNumber;
		unsigned int			uiCurrentRequestNumber;

		// ������������ ������ ����: view � ���������� ����� ������� � ���� ����� � ��� ������� ������� � ���� ��������
		std::map<std::pair<unsigned int, unsigned int>, RequestLogRecording> requestLog;
	};

	std::shared_ptr<std::vector<SBFTDataPackage*>>	m_transactionPool;
	std::shared_ptr<SBFTState>						m_state;
	std::shared_ptr<std::mutex>						m_pAddingBlockInProcessMutex;
	std::shared_ptr<NodeLogger>						m_pLogger;
	Role											m_role;
};

