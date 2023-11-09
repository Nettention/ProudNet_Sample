using System;
using Nettention.Proud;

namespace SimpleCSharp
{
    class Program
    {
        // RMI stub instance
        // For details, check client source code first.
        //
        // 클라이언트에서 오는 메시지를 받기 위한 RMI Stub
        static Simple.Stub g_Stub = new Simple.Stub();

        // RMI proxy
        static Simple.Proxy g_Proxy = new Simple.Proxy();

        // a P2P group where all clients are in.
        //
        // 모든 클라이언트가 속해있는 p2p group
        static HostID g_groupHostID = HostID.HostID_None;

        static void InitStub()
        {
            g_Stub.Chat = (Nettention.Proud.HostID remote, Nettention.Proud.RmiContext rmiContext, String a, int b, float c) =>
                {
                    Console.Write("[Server] chat message received :");
                    Console.Write(" a={0} b={1} c={2}\n", a, b, c);

                    // Echo chatting message which received from server to client.
                    // 
                    // 서버가 받은 메시지를 클라이언트로 다시 보내준다.
                    g_Proxy.ShowChat(remote, RmiContext.ReliableSend, a, b + 1, c + 1);

                    return true;
                };

        }


        internal static void StartServer(NetServer server, Nettention.Proud.StartServerParameter param)
        {
            if ((server == null) || (param == null))
            {
                throw new NullReferenceException();
            }

            try
            {
                server.Start(param);
            }
            catch (System.Exception ex)
            {
                Console.WriteLine("Failed to start server~!!" + ex.ToString());
            }

            Console.WriteLine("Succeed to start server~!!\n");
        }

        static void Main(string[] args)
        {
            // Network server instance.
            //
            // CNetServer 인스턴스 생성
            NetServer srv = new NetServer();

            // set a routine which is executed when a client is joining.
            // clientInfo has the client info including its HostID.
            //
            // 클라이언트가 서버에 접속했을 때 실행할 로직을 설정한다.
            srv.ClientJoinHandler = (clientInfo) =>
            {
                Console.Write("Client {0} connected.\n", clientInfo.hostID);
            };

            // set a routine for client leave event.
            // 
            // 클라이언트 서버 접속이 끊어졌을 때 실행할 로직을 설정한다.
            srv.ClientLeaveHandler = (clientInfo, errorInfo, comment) =>
            {
                Console.Write("Client {0} disconnected.\n", clientInfo.hostID);
            };

            InitStub();

            // Associate RMI proxy and stub instances to network object.
            //
            // 생성된 CNetServer 인스턴스에 proxy와 stub을 등록한다.
            srv.AttachStub(g_Stub);
            srv.AttachProxy(g_Proxy);

            var p1 = new StartServerParameter();
            p1.protocolVersion = new Nettention.Proud.Guid(Vars.m_Version); // This must be the same to the client. => 클라이언트와 같아야 함.
            p1.tcpPorts.Add(Vars.m_serverPort); // TCP listening endpoint => tcp 연결 들어오는 port

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
                srv.Start(p1);
            }
            catch (Exception e)
            {
                Console.Write("Server start failed: {0}\n", e.ToString());
                return;
            }

            Console.Write("Server started. Enterable commands:\n");
            Console.Write("1: Creates a P2P group where all clients join.\n");
            Console.Write("2: Sends a message to P2P group members.\n");
            Console.Write("q: Quit.\n");

            string userInput;

            while (true)
            {
                // get user input
                // 
                // 유저 인풋을 가져옴.
                userInput = Console.ReadLine();

                if (userInput == "1")
                {
                    // get all client HostID array.
                    //
                    // 모든 클라이언트 호스트 아이디를 가져온다.
                    HostID[] clients = srv.GetClientHostIDs();

                    // create a P2P group where all clients are in.
                    //
                    // 모든 클라이언트들이 속해있는 p2p group을 만든다.
                    g_groupHostID = srv.CreateP2PGroup(clients, new ByteArray());
                }
                else if (userInput == "2")
                {
                    // send an RMI message to every client.
                    //
                    // 모든 클라이언트들에게 RMI 메시지를 전송한다.
                    g_Proxy.SystemChat(g_groupHostID, RmiContext.ReliableSend, "Hello~~~!");
                }
                else if (userInput == "3")
                {
                    // destroy the P2P group.
                    //
                    // P2P 그룹을 파괴한다.
                    srv.DestroyP2PGroup(g_groupHostID);
                    g_groupHostID = HostID.HostID_None;
                }
                else if (userInput == "q")
                {
                    // exit program.
                    break;
                }
            }

            srv.Stop();
        }
    }
}
