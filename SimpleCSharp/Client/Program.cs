using System;
using System.Threading;
using Nettention.Proud;

namespace SimpleCSharp
{
    class Program
    {
        // Protects all variable here.
        // If you are developing a game program or an app with fast-running loop,
        // this is not needed, as you are making a single threaded app.
        // This is required as this sample program uses two threads.
        // One is for waiting user input, the other is for processing
        // received messages and events.
        //
        // 싱글 스레드로 프로그래밍할 경우 이 변수는 필요하지 않습니다.
        // 두개 이상의 스레드를 사용할 경우 이 변수를 사용해 lock 처리를 합니다.
        // 하나는 유저의 input을 기다리기 위함이고, 나머지 하나는 메시지 받기와 이벤트를 처리하기 위함이다.
        static object g_critSec = new object();

        // RMI proxy.
        // RMI proxy is used for sending messages aka.
        // calling a function which resides other process.
        //
        // RMI 프록시는 메시지 메시지를 보내는 데 사용된다.
        // 함수 호출은 다른 프로세스에서 실행된다.
        static Simple.Proxy g_Proxy = new Simple.Proxy();

        // RMI stub for receiving messages.
        //
        // RMI stub은 메시지를 받는 데 사용된다.
        static Simple.Stub g_Stub = new Simple.Stub();

        // 각 stub 함수 송신 시 실행할 기능을 정의한 함수.
        static void InitStub()
        {
            g_Stub.P2PChat = (remote, rmiContext, a, b, c) =>
            {
                lock (g_critSec)
                {
                    Console.Write("[Client] P2PChat : a={0}, b={1}, c={2}, relay={3}, rmiID={4}\n", a, b, c, rmiContext.relayed, rmiContext.rmiID);
                }
                return true;
            };

            g_Stub.ShowChat = (remote, rmiContext, a, b, c) =>
            {
                lock (g_critSec)
                {
                    Console.Write("[Client] ShowChat : a={0}, b={1}, c={2}, rmiID={3}\n", a, b, c, rmiContext.rmiID);
                }
                return true;
            };

            g_Stub.SystemChat = (remote, rmiContext, txt) =>
            {
                lock (g_critSec)
                {
                    Console.Write("[Client] SystemChat : txt={0}, rmiID={1}\n", txt, rmiContext.rmiID);
                }
                return true;
            };
        }

        static void Main(string[] args)
        {
            InitStub();

            // Network client object.
            // NetClient 인스턴스
            NetClient netClient = new NetClient();

            // set to false to exit the main loop.
            // 
            // 메인 루프를 끝내려 할 때 false로 설정한다.
            bool keepWorkerThread = true;
            // set to true if server connection is established.
            // 
            // 서버 연결이 되면 true로 설정한다.
            bool isConnected = false;
            // changed if another P2P peer joined.
            // 
            // 또 다른 p2p 연결이 들어오면 해당 그룹의 아이디로 변경된다.
            HostID recentP2PGroupHostID = HostID.HostID_None;

            // set a routine which is run when the server connection attempt
            // is success or failed.
            // 
            // 서버 연결이 완료된 후 호출되는 이벤트로 
            // 아래와 같이 성공 실패에 따른 로직을 실행할 수 있다.
            netClient.JoinServerCompleteHandler = (info, replyFromServer) =>
            {
                // as here is running in 2nd thread, lock is needed for console print.
                // 
                // 이렇게 두번째 스레드에서 실행될 때 console print에 대한 lock이 필요해진다.
                lock (g_critSec)
                {
                    if (info.errorType == ErrorType.Ok)
                    {
                        Console.Write("Succeed to connect server. Allocated hostID={0}", netClient.GetLocalHostID());
                        isConnected = true;
                        // send a message.
                        //
                        // 메시지 전송
                        g_Proxy.Chat(HostID.HostID_Server, // send destination => 대상 HostID
                        RmiContext.ReliableSend, // how to send => 어떤 방식으로 보낼지
                        "Hello ProudNet~!!!.", 333, 22.33f); // user defined parameters => 유저가 정의한 파라미터
                    }
                    else
                    {
                        // connection failure.
                        //
                        // 연결 실패시 출력할 메시지
                        Console.Write("Failed to connect server.\n");
                        Console.WriteLine("errorType = {0}, detailType = {1}, comment = {2}", info.errorType, info.detailType, info.comment);
                    }
                }
            };

            // set a routine for network disconnection.
            // 
            // 서버 연결이 끊어졌을 때 실행할 로직
            netClient.LeaveServerHandler = (errorInfo) =>
            {
                // lock is needed as above.
                lock (g_critSec)
                {
                    // show why disconnected.
                    //
                    // 실패 이유를 출력
                    Console.Write("OnLeaveServer: {0}\n", errorInfo.comment);

                    // let main loop exit
                    // 메인 스레드 종료를 위한 변수 수정
                    isConnected = false;
                    keepWorkerThread = false;
                }
            };

            // set a routine for P2P member join (P2P available)
            //
            // p2p 그룹에 새로운 멤버가 추가됐을 때 실행할 로직
            netClient.P2PMemberJoinHandler = (memberHostID, groupHostID, memberCount, customField) =>
            {
                // lock is needed as above.
                lock (g_critSec)
                {
                    // memberHostID = P2P connected client ID
                    // groupHostID = P2P group ID where the P2P peer is in.
                    //
                    // memberHostID = p2p 연결된 클라이언트 아이디
                    // groupHostID = p2p 연결된 그룹 아이디
                    Console.Write("[Client] P2P member {0} joined group {1}.\n", memberHostID, groupHostID);

                    g_Proxy.P2PChat(memberHostID, RmiContext.ReliableSend,
                    "Welcome!", 5, 7);
                    recentP2PGroupHostID = groupHostID;
                }
            };

            // called when a P2P member left.
            // 
            // p2p 멤버 접속이 끊겼을 때 실행할 로직
            netClient.P2PMemberLeaveHandler = (memberHostID, groupHostID, memberCount) =>
            {
                Console.Write("[Client] P2P member {0} left group {1}.\n", memberHostID, groupHostID);
            };

            // attach RMI proxy and stub to client object.
            //
            // NetClient 인스턴스에 proxy와 stub 연결
            netClient.AttachProxy(g_Proxy);	    // Client-to-server => 클라이언트에서 서버로
            netClient.AttachStub(g_Stub);		// server-to-client => 서버에서 클라이언트로

            NetConnectionParam cp = new NetConnectionParam();
            // Protocol version which have to be same to the value at the server.
            //
            // 서버와 동일한 protocol version
            cp.protocolVersion.Set(SimpleCSharp.Vars.m_Version);

            // server address
            cp.serverIP = "localhost";

            // server port
            cp.serverPort = (ushort)SimpleCSharp.Vars.m_serverPort;

            // Starts connection.
            // This function returns immediately.
            // Meanwhile, connection attempt is process in background
            // and the result is notified by OnJoinServerComplete event.
            //
            // 서버 연결 시작
            // 이 함수는 즉시 리턴됩니다.
            // 그동안 백그라운드에서 연결을 시도하고,
            // OnJoinServerComplete 이벤트에 의해 접속 완료를 알 수 있습니다.
            netClient.Connect(cp);

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
            Thread workerThread = new Thread(() =>
            {
                while (keepWorkerThread)
                {
                    // Prevent CPU full usage.
                    // 
                    // cpu 사용률을 줄이기 위해 thread sleep을 건다.
                    Thread.Sleep(10);

                    // process received RMI and events.
                    // 
                    // RMI와 이벤트 수신을 받기 위한 과정
                    netClient.FrameMove();
                }
            });

            workerThread.Start();

            Console.Write("a: Send a P2P message to current P2P group members except for self.\n");
            Console.Write("q: Exit.\n");

            while (keepWorkerThread)
            {
                // get user input
                string userInput = Console.ReadLine();

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
                        lock (g_critSec)
                        {
                            // Sends a P2P message to everyone in a group
                            // specified at recentP2PGroupHostID.
                            // 
                            // recentP2PGroupHostID로 특정되는 그룹의 모든 사람들에게 p2p 메시지를 보낸다.
                            RmiContext sendHow = RmiContext.ReliableSend;
                            sendHow.enableLoopback = false; // don't sent to myself.
                            g_Proxy.P2PChat(recentP2PGroupHostID, sendHow,
                            "Welcome ProudNet!!", 1, 1);
                        }
                    }
                    else
                    {
                        // We have no server connection. Show error.
                        // 
                        // connection이 없다면 에러를 출력한다.
                        Console.Write("Not yet connected.\n");
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
            netClient.Disconnect();

        }
    }
}
