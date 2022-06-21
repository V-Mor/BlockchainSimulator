#include "StdAfx.h"

#include <iomanip>
#include <chrono>
#include <fstream>

#include "PBFTNodeFactory.h"
#include "ECDSASignatureManager.h"

#include <windows.h>
#include <random>

double INTERVAL_AVERAGE;
unsigned int BLOCK_SIZE;
unsigned int RESENDING_TIMEOUT = 10; // секунды
unsigned int const SIMULATION_TIME = 30; // минуты


int main()
{
	// Establishing a process priority for better performance
	//SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

	for (int nodesNumber = 25; nodesNumber <= 25; nodesNumber += 10)
	{
		for (int j = 25; j <= 30; j += 5)
		{

			BLOCK_SIZE = j + 2;
			/*double* pIntAv = INTERVAL_AVERAGE;
			*pIntAv = 5.0 * j;*/

			//RESENDING_TIMEOUT = j * 5;

			DigitalSignatureManager::Ptr signatureManager(new ECDSASignatureManager());

			// Factory type only is responsible for performing protocol
			PBFTNodeFactory factory(signatureManager);

			std::vector<Node::Ptr> pNodes;

			// 34 (it is one of the variative parameters) PBFT-nodes run (f = 11)
			for (int i = 0; i < nodesNumber; ++i)
				pNodes.push_back(factory.RunNewNode());

			//std::mt19937::result_type seed = time(0);
			//auto getRandomInt = std::bind(std::uniform_int_distribution<int>(1, 21), std::mt19937(seed));
			//// Next 12 (it is one of the variative parameters) random nodes is stopped 
			//// (it is possible that lower number of nodes will be stopped in case when the same numbers will be generated) 
			//for (int i = 0; i < 7; ++i)
			//{
			//	pNodes[getRandomInt()]->Stop();
			//}

			std::string logFileName("nodes" + 
				std::to_string(nodesNumber) + 
				"_blocksize" + 
				std::to_string(BLOCK_SIZE) + 
				"_timeout" + 
				std::to_string(RESENDING_TIMEOUT) + 
				"between.log");

			// Waiting for the end of the experiment (possibly logging some information)
			for (size_t i = 0; i < SIMULATION_TIME; ++i)
			{
				std::this_thread::sleep_for(std::chrono::minutes(1));

				/*for (Node::Ptr pNode : pNodes)
				{
					std::cout << reinterpret_cast<PBFTNode*>(pNode.get())->GetQueueLength() << " ";
				}

				std::cout << std::endl;*/

				/*std::ofstream log(logFileName, std::ios::app);

				for (Node::Ptr pNode : pNodes)
				{
					log << (int)pNode->GetStoredBlocksNumber() << " ";
				}
				log << "\n";
				log.close();*/

			}

			for (Node::Ptr pNode : pNodes)
			{
				//std::cout << reinterpret_cast<PBFTNode*>(pNode.get())->CountLostTransactions() << std::endl;
				//std::cout << reinterpret_cast<PBFTNode*>(pNode.get())->GetTotalResendingsNumber() << std::endl;
				std::ofstream log(logFileName, std::ios::app);

				for (Node::Ptr pNode : pNodes)
				{
					log << reinterpret_cast<PBFTNode*>(pNode.get())->GetTotalResendingsNumber() << " ";
				}
				log << "\n";
				log.close();
			}

			std::cout << "Stopping nodes...\n";

			for (Node::Ptr pNode : pNodes)
			{
				pNode->Stop();
			}

			std::cout << "Nodes stopped\n";

			//pNodes.clear();
		}
	}

	std::cout << "Program finished\n\a";
	getchar();
	return 0;
}