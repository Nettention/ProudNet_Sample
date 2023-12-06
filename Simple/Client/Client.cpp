#include "stdafx.h"

#include "ProudNetClient.h"

using namespace Proud;
using namespace std;

#include "../Common/Vars.h"
#include "../Common/Simple_common.cpp"
#include "../Common/Simple_proxy.cpp"
#include "../Common/Simple_stub.cpp"

/* Protects all variable here. 
If you are developing a game program or an app with fast-running loop, 
this is not needed, as you are making a single threaded app.
This is required as this sample program uses two threads. 
One is for waiting user input, the other is for processing 
received messages and events. */

/* 
싱글 스레드로 프로그래밍할 경우 이 변수는 필요하지 않습니다.
두개 이상의 스레드를 사용할 경우 이 변수를 사용해 lock 처리를 합니다.
하나는 유저의 input을 기다리기 위함이고, 나머지 하나는 메시지 받기와 이벤트를 처리하기 위함이다.
*/
CriticalSection g_critSec;

/* Client-to-server RMI proxy.
RMI proxy is used for sending messages aka. 
calling a function which resides other process.
*/

/* 클라이언트 -> 서버 RMI proxy.
RMI 프록시는 메시지 메시지를 보내는 데 사용된다.
함수 호출은 다른 프로세스에서 실행된다.
*/
Simple::Proxy g_SimpleProxy;

/* RMI stub for receiving messages.
Unlike RMI proxy, it is derived and RMI function is 
implemented by function override. */

/* 메시지를 받기 위한 RMI stub
RMI proxy와는 다르게 함수 오버라이딩 후 사용한다.
*/
class SimpleStub : public Simple::Stub
{
public:
	DECRMI_Simple_ShowChat;
	DECRMI_Simple_SystemChat;

	DECRMI_Simple_P2PChat;
};

// RMI stub instance.
// 
// RMI stub 인스턴스
SimpleStub g_SimpleStub;

// RMI function implementation that you define.
// We use DEFRMI_GlobalName_FunctionName for easier coding.
// 
// 직접 정의한 RMI 함수
// 쉬운 네이밍을 위해 다음과 같은 규칙을 사용합니다. => DEFRMI_GlobalName_FunctionName
DEFRMI_Simple_P2PChat(SimpleStub)
{
	CriticalSectionLock lock(g_critSec, true);
	cout << "[Client] P2PChat: relayed="
		<< (rmiContext.m_relayed ? "true" : "false")
		<< " a=" << string(a)
		<< " b=" << b
		<< " c=" << c
		<< endl;

	// no meaning, but must return true always.
	// 
	// 따로 의미는 없으나 반드시 true를 리턴해주어야 함.
	return true;
}

DEFRMI_Simple_ShowChat(SimpleStub)
{
	CriticalSectionLock lock(g_critSec, true);
	cout << "[Client] ShowChat: a=" << (string)a << ",b=" << b << ",c=" << c << endl;
	return true;
}

DEFRMI_Simple_SystemChat(SimpleStub)
{
	CriticalSectionLock lock(g_critSec, true);
	cout << "[Client] SystemChat: txt=" << (string)txt << endl;
	return true;
}

int main(int argc, char* argv[])
{
	// Network client object.
	// 
	// 프라우드넷 client object
	shared_ptr<CNetClient> netClient(CNetClient::Create());

	// set to false to exit the main loop.
	// 
	// 메인 루프를 끝내려 할 때 false로 설정한다.
	volatile bool keepWorkerThread = true;
	// set to true if server connection is established.
	// 
	// 서버 연결이 되면 true로 설정한다.
	bool isConnected = false;
	// changed if another P2P peer joined.
	// 
	// 또 다른 p2p 연결이 들어오면 해당 그룹의 아이디로 변경된다.
	HostID recentP2PGroupHostID = HostID_None;

	// set a routine which is run when the server connection attempt
	// is success or failed.
	// 
	// 서버 연결이 완료된 후 호출되는 이벤트로 
	// 아래와 같이 성공 실패에 따른 로직을 실행할 수 있다.
	netClient->OnJoinServerComplete = 
		[&](ErrorInfo *info, const ByteArray &replyFromServer)
	{
		// as here is running in 2nd thread, lock is needed for console print.
		// 
		// 이렇게 두번째 스레드에서 실행될 때 console print에 대한 lock이 필요해진다.
		CriticalSectionLock lock(g_critSec, true);

		if (info->m_errorType == ErrorType_Ok)
		{
			// connection successful.
			// 
			// 연결에 성공했을 때
			printf("Succeed to connect server. Allocated hostID=%d\n", netClient->GetLocalHostID());
			isConnected = true;

			// send a message.
			// 
			// 메시지 전송
			g_SimpleProxy.Chat(HostID_Server, // send destination 대상 호스트 아이디
				RmiContext::ReliableSend, // how to send 어떤 방식으로 보낼지
				_PNT("Hello ProudNet~!!!."), 333, 22.33f); // user defined parameters 유저가 정의한 파라미터

		}
		else
		{
			// connection failure.
			// 
			// 연결 실패했을 때
			cout << "Failed to connect to server.\n";
		}
	};

	// set a routine for network disconnection.
	// 
	// 서버 연결이 끊어졌을 때 실행할 로직
	netClient->OnLeaveServer = [&](ErrorInfo *errorInfo) {

		// lock is needed as above.
		// 
		// 마찬가지로 lock이 필요함.
		CriticalSectionLock lock(g_critSec, true);

		// show why disconnected.
		// 
		// 연결이 끊어진 이유 출력
		cout << "OnLeaveServer: " << StringT2A(errorInfo->m_comment).GetString() << endl;

		// let main loop exit
		// 
		// 메인 루프 종료
		isConnected = false;
		keepWorkerThread = false;
	};

	// set a routine for P2P member join (P2P available)
	// 
	// 새로운 p2p 연결이 들어왔을 때 실행될 로직
	// memberHostID : p2p 연결된 클라이언트 아이디
	// groupHostID : p2p 연결된 그룹 아이디
	netClient->OnP2PMemberJoin = [&](HostID memberHostID, HostID groupHostID,int memberCount, const ByteArray &customField) {
		CriticalSectionLock lock(g_critSec, true);

		printf("[Client] P2P member %d joined group %d.\n", memberHostID, groupHostID);

		g_SimpleProxy.P2PChat(memberHostID, RmiContext::ReliableSend,
			_PNT("Welcome!"), 5, 7.f);
		recentP2PGroupHostID = groupHostID;
	};

	// called when a P2P member left.
	// 
	// p2p 연결이 끊겼을 때 실행될 로직
	netClient->OnP2PMemberLeave = [](HostID memberHostID, HostID groupHostID,int memberCount) {
		printf("[Client] P2P member %d left group %d.\n", memberHostID, groupHostID);
	};

	// attach RMI proxy and stub to client object.
	// 
	// CNetClient에 유저가 만든 proxy와 stub을 등록한다.
	netClient->AttachProxy(&g_SimpleProxy);	
	netClient->AttachStub(&g_SimpleStub);	

	CNetConnectionParam cp;

	// Protocol version which have to be same to the value at the server.
	// 
	// 서버와 같은 protocol 버전을 입력해야 한다. 아예 입력하지 않을 수도 있음.
	cp.m_protocolVersion = g_Version;
    cp.m_closeNoPingPongTcpConnections=false;

	// server address
	if (argc > 1)
	{
		cp.m_serverIP = StringA2T(argv[1]);
	}
	else
	{
		cp.m_serverIP = _PNT("localhost");
	}

	cp.m_serverPort = g_ServerPort;		

	// Starts connection.
	// This function returns immediately.
	// Meanwhile, connection attempt is process in background
	// and the result is notified by OnJoinServerComplete event.
	// 
	// 연결 시작
	// 이 함수는 즉시 return 됩니다.
	// 그동안 백그라운드에서 연결을 시도하고,
	// 결과는 OnJoinServerComplete 이벤트에 의해서 알려집니다.
	netClient->Connect(cp);

	// As we have to be notified events and message receivings,
	// We call FrameMove function for every short interval.
	// If you are developing a game, call FrameMove
	// without doing any sleep. 
	// If you are developing an app, call FrameMove
	// in another thread or your timer callback routine.
	//
	// 메시지 수신과 이벤트 알림을 받듯, 지정한 시간마다 framemove 함수를 호출한다.
	// 게임 개발이라면 따로 thread sleep없이 framemove를 호출한다.
	// 일반 앱 개발이라면 또 다른 스레드에서 호출하거나 timer를 지정해 호출한다.
	Proud::Thread workerThread([&](){
		while (keepWorkerThread)
		{
			// Prevent CPU full usage.
			// 
			// cpu 사용률을 줄이기 위해 thread sleep을 건다.
			Proud::Sleep(10);		

			// process received RMI and events.
			// 
			// RMI와 이벤트 수신을 받기 위한 과정
			netClient->FrameMove();
		}
	});
	workerThread.Start();

	cout << "a: Send a P2P message to current P2P group members except for self.\n";
	cout << "q: Exit.\n";

	while (keepWorkerThread)
	{
		// get user input
		// 
		// 유저 input을 얻는다.
		string userInput;
		cin >> userInput;

		// received user command. process it.
		// 
		// 유저 입력대로 처리하는 코드.
		if (userInput == "q")
		{
			// let worker thread exit.
			// 
			// 매인 스레드가 종료되게 한다.
			keepWorkerThread = false;
		}
		else if (userInput == "a")
		{
			if (isConnected)
			{
				// As we access recentP2PGroupHostID which is also accessed by
				// another thread, lock it.
				// 
				// 또 다른 스레드에 의해 접근되는 recentP2PGroupHostID에 접근하기 위해 락을 건다.
				CriticalSectionLock lock(g_critSec, true);

				// Sends a P2P message to everyone in a group 
				// specified at recentP2PGroupHostID.
				// 
				// recentP2PGroupHostID로 특정되는 그룹의 모든 사람들에게 p2p 메시지를 보낸다.
				RmiContext sendHow = RmiContext::ReliableSend;
				sendHow.m_enableLoopback = false; // don't sent to myself.
				g_SimpleProxy.P2PChat(recentP2PGroupHostID, sendHow, 
					_PNT("Welcome ProudNet!!"), 1, 1);
			}
			else
			{
				// We have no server connection. Show error.
				// 
				// connection이 없다면 에러를 출력한다.
				cout << "Not yet connected.\n";
			}
		}
	}

	// Waits for 2nd thread exits.
	// 
	// 다음 스레드 연결을 기다린다.
	workerThread.Join();

	// Disconnects.
	// Note: deleting NetClient object automatically does it.
	//
	// 연결을 종료한다.
	// 자동으로 CNetClient 오브젝트를 지운다.
	netClient->Disconnect();

	return 0;
}
