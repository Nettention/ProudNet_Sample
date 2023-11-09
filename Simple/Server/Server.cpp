#include "stdafx.h"

#include "ProudNetServer.h"

using namespace std;
using namespace Proud;

#include "../Common/Vars.h"
#include "../Common/Simple_common.cpp"
#include "../Common/Simple_proxy.cpp"
#include "../Common/Simple_stub.cpp"

// Server-to-client RMI stub
// For details, check client source code first.
//
// 클라이언트에서 오는 메시지를 받기 위한 RMI Stub
class SimpleStub: public Simple::Stub
{
public:
	DECRMI_Simple_Chat;
};

// and its instance
//
// Stub 인스턴스
SimpleStub g_SimpleStub;

// Server-to-client RMI proxy
//
// 서버에서 클라이언트로 전송을 위한 RMI proxy
Simple::Proxy g_SimpleProxy;

// a P2P group where all clients are in.
//
// 모든 클라이언트가 속해있는 p2p group
HostID g_groupHostID = HostID_None;

DEFRMI_Simple_Chat(SimpleStub)
{
	cout << "[Server] chat message received :";
	cout << " a=" << string(a);
	cout << " b=" << b;
	cout << " c=" << c;
	cout << endl;

	// Echo chatting message which received from server to client.
	// 
	// 서버가 받은 메시지를 클라이언트로 다시 보내준다.
	g_SimpleProxy.ShowChat(remote, RmiContext::ReliableSend, a, b + 1, c + 1);

	return true;
}


int main(int argc, char* argv[])
{
	// Network server instance.
	//
	// CNetServer 인스턴스 생성
	shared_ptr<CNetServer> srv(CNetServer::Create());

	// set a routine which is executed when a client is joining.
	// clientInfo has the client info including its HostID.
	//
	// 클라이언트가 서버에 접속했을 때 실행할 로직을 설정한다.
	srv->OnClientJoin = [](CNetClientInfo *clientInfo){
		printf("Client %d connected.\n", clientInfo->m_HostID);
	};

	// set a routine for client leave event.
	// 
	// 클라이언트 서버 접속이 끊어졌을 때 실행할 로직을 설정한다.
	srv->OnClientLeave = [](CNetClientInfo *clientInfo, ErrorInfo *errorInfo, const ByteArray& comment){
		printf("Client %d disconnected.\n", clientInfo->m_HostID);
	};

	// Associate RMI proxy and stub instances to network object.
	//
	// 생성된 CNetServer 인스턴스에 proxy와 stub을 등록한다.
	srv->AttachStub(&g_SimpleStub);
	srv->AttachProxy(&g_SimpleProxy);

	CStartServerParameter p1;
	p1.m_protocolVersion = g_Version; // This must be the same to the client. => 클라이언트와 같아야 함.
	p1.m_tcpPorts.Add(g_ServerPort); // TCP listening endpoint => tcp 연결 들어오는 port

	ErrorInfoPtr err;
	try
	{
		/* Starts the server.
		This function throws an exception on failure.
		Note: As we specify nothing for threading model,
		RMI function by message receive and event callbacks are
		called in a separate thread pool.
		You can change the thread model. Check out the help pages for details. */

		/* 서버 시작
		이 함수는 실패시 exception을 발생시킨다.
		특별히 threading model을 구체화하지 않는다면
		메시지 송신에 의한 RMI 함수와 이벤트 콜백은 나눠진 thread pool에서 호출된다.
		thread model을 따로 지정할 수 있는데 해당 부분은 도움말을 참고하면 된다. https://guide.nettention.com/cpp_ko#thread_pool_sharing
		*/
		srv->Start(p1);
	}
	catch (Exception &e)
	{
		cout << "Server start failed: " << e.what() << endl;
		return 0;
	}

	puts("Server started. Enterable commands:\n");
	puts("1: Creates a P2P group where all clients join.\n");
	puts("2: Sends a message to P2P group members.\n");
	puts("q: Quit.\n");

	string userInput;

	while (true)
	{
		// get user input
		// 
		// 유저 인풋을 가져옴.
		cin >> userInput;

		if ( userInput == "1" )
		{
			// get all client HostID array.
			//
			// 모든 클라이언트 호스트 아이디를 가져온다.
			vector<HostID> clients;
			int noofClients = srv->GetClientCount();
			clients.resize(noofClients);
			
			if (clients.size() == 0)
				continue;

			int listCount = srv->GetClientHostIDs(&clients[0], noofClients);

			// create a P2P group where all clients are in.
			//
			// 모든 클라이언트들이 속해있는 p2p group을 만든다.
			g_groupHostID = srv->CreateP2PGroup(&clients[0], clients.size());
		}
		else if (userInput == "2")
		{
			// send an RMI message to every client.
			//
			// 모든 클라이언트들에게 RMI 메시지를 전송한다.
			g_SimpleProxy.SystemChat(g_groupHostID, RmiContext::ReliableSend, _PNT("Hello~~~!"));
		}
		else if (userInput == "3")
		{
			// destroy the P2P group.
			//
			// P2P 그룹을 파괴한다.
			srv->DestroyP2PGroup(g_groupHostID);
			g_groupHostID = HostID_None;
		}
		else if (userInput == "q")
		{
			// exit program.
			break;
		}
	}

	return 0;
}



