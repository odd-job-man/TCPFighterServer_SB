#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <map>
#include <queue>
#include <set>
#include "Contents.h"
#include "Network.h"
#include "SerializeBuffer.h"

#define LOG
#pragma comment(lib,"ws2_32.lib")


#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
SOCKET g_listenSock;
std::map<int, Session*>g_sessionMap;
extern std::map<int, ClientInfo*> g_clientMap;
constexpr int SERVERPORT = 5000;
constexpr int MAX_USER_NUM = 60;
std::queue<int> id_q;
std::set<int> g_disconnected_id_set;
SerializeBuffer g_sb;

void ClearClientInfo();
void ClearSessionInfo()
{
	for (auto iter = g_sessionMap.begin(); iter != g_sessionMap.end();)
	{
		delete iter->second;
		iter = g_sessionMap.erase(iter);
	}
	ClearClientInfo();
	g_sessionMap.clear();
	g_disconnected_id_set.clear();
	closesocket(g_listenSock);
	g_sb.FreeBuffer();
	WSACleanup();
}

bool AcceptProc();
void RecvProc(IN Session* pSession);
void SendProc(IN Session* pSession);
bool EnqNewRBForOtherCreate(IN Session* pNewUser);
void EnqPacketBroadCast(IN char* pPacket, IN const size_t packetSize, IN OPTIONAL const int pTargetIdToExcept);
void InitClientInfo(OUT ClientInfo* pCI, IN int id);

bool SC_CREATE_MY_CHARACTER(IN SerializeBuffer& sb, int id, char direction, USHORT x, USHORT y, char hp);
bool SC_CREATE_OTHER_CHARACTER(SerializeBuffer& sb, int createId, int destId, char direction, USHORT x, USHORT y, char hp);



inline bool isDeletedSession(IN const int id)
{
	return g_disconnected_id_set.find(id) != g_disconnected_id_set.end();
}
bool EnqPacketUnicast(IN const int id,IN char* pPacket,IN const size_t packetSize)
{
	if (isDeletedSession(id))
		return false;

	RingBuffer& sendRB = g_sessionMap.find(id)->second->sendBuffer;

	int EnqRet = sendRB.Enqueue(pPacket, packetSize);
	if (EnqRet == 0)
	{
#ifdef LOG
		wprintf(L"Session ID : %d\tsendRingBuffer Full\t", id);
#endif
		disconnect(id);
		return false;
	}
#ifdef LOG
	wprintf(L"Session ID : %d, sendRB Enqueue Size : %d\tsendRB FreeSize : %d\n", id, EnqRet,sendRB.GetFreeSize());
#endif
	return true;
}


bool NetworkInitAndListen()
{
	int bindRet;
	int listenRet;
	int ioctlRet;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		wprintf(L"WSAStartup() func error code : %d\n", WSAGetLastError());
		return false;
	}
#ifdef LOG
	wprintf(L"WSAStartup #\n");
#endif

	g_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_listenSock == INVALID_SOCKET)
	{
		wprintf(L"socket() func error code : %d\n", WSAGetLastError());
		return false;
	}

	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(SERVERPORT);

	bindRet = bind(g_listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (bindRet == SOCKET_ERROR)
	{
		wprintf(L"bind() func error code : %d\n", WSAGetLastError());
		return false;
	}
#ifdef LOG
	wprintf(L"Bind OK # Port : %d\n", SERVERPORT);
#endif

	listenRet = listen(g_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		wprintf(L"listen() func error code : %d\n", WSAGetLastError());
		return false;
	}
#ifdef LOG
	wprintf(L"Listen OK #\n");
#endif

	u_long iMode = 1;
	ioctlRet = ioctlsocket(g_listenSock, FIONBIO, &iMode);
	if (ioctlRet != 0)
	{
		wprintf(L"ioctlsocket() func error code : %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool NetworkProc()
{
	static bool isFirst = true;
	int selectRet;
	fd_set readSet;
	fd_set writeSet;
	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	FD_SET(g_listenSock, &readSet);
	for (auto iter = g_sessionMap.begin(); iter != g_sessionMap.end(); ++iter)
	{
		FD_SET(iter->second->clientSock, &readSet);
		if (iter->second->sendBuffer.GetUseSize() > 0)
		{
			FD_SET(iter->second->clientSock, &writeSet);
		}
	}

	if (isFirst)
	{
		for (int i = 0; i < MAX_USER_NUM; ++i)
		{
			id_q.push(i);
		}
		g_sb.AllocBuffer();
		isFirst = false;
	}

	SOCKADDR_IN clientAddr;
	int addrlen = sizeof(clientAddr);
	timeval tv{ 0,0 };
	selectRet = select(0, &readSet, &writeSet, nullptr, &tv);
	if (selectRet == 0)
	{
		return false;
	}
	if (selectRet == SOCKET_ERROR)
	{
		wprintf(L"select func error code : %d\n", WSAGetLastError());
		return false;
	}
	if (FD_ISSET(g_listenSock, &readSet))
	{
		AcceptProc();
		--selectRet;
	}

	for (auto iter = g_sessionMap.begin(); iter != g_sessionMap.end() && selectRet > 0; ++iter)
	{
		Session* pSession = iter->second;
		if (FD_ISSET(pSession->clientSock, &readSet))
		{
			RecvProc(pSession);
			--selectRet;
		}
		if (FD_ISSET(iter->second->clientSock, &writeSet))
		{
			SendProc(pSession);
			--selectRet;
		}
	}
	return true;
}
void NotifyMovingToNew(int newID);
bool AcceptProc()
{
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrlen = sizeof(clientAddr);
	clientSock = accept(g_listenSock, (SOCKADDR*)&clientAddr, &addrlen);
	if (clientSock == INVALID_SOCKET)
	{
		wprintf(L"accept func error code : %d\n", WSAGetLastError());
		__debugbreak();
		return false;
	}
	if (g_sessionMap.size() >= MAX_USER_NUM)
	{
		// �������
		closesocket(clientSock);
		return false;
	}
	int newID = id_q.front();
	id_q.pop();
#ifdef LOG
	WCHAR szIpAddr[MAX_PATH];
	InetNtop(AF_INET, &clientAddr.sin_addr, szIpAddr, _countof(szIpAddr));
	wprintf(L"Connect # ip : %s / SessionId:%d\n", szIpAddr, newID);
#endif 
	Session* pNewSession = new Session{ clientSock,newID };
	g_sessionMap.insert(std::pair<int, Session*>{newID, pNewSession});

	ClientInfo* pNewClientInfo = new ClientInfo;
	InitClientInfo(pNewClientInfo, newID);
	g_clientMap.insert(std::pair<int, ClientInfo*>{newID, pNewClientInfo});
#ifdef LOG
	wprintf(L"Session ID : %d, PACKET_SC_CREATE_MY_CHARACTER\n", newID);
#endif
	if (!SC_CREATE_MY_CHARACTER(g_sb, newID, pNewClientInfo->viewDir, pNewClientInfo->x, pNewClientInfo->y, pNewClientInfo->hp))
	{
		return false;
	}
#ifdef LOG
	wprintf(L"Session ID : %d, PACKET_SC_CREATE_OHTER_CHARACTER\t New To Other\n", newID);
#endif
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pCI = iter->second;
		if (pCI->ID != newID)
		{
			SC_CREATE_OTHER_CHARACTER(g_sb, newID, pCI->ID, pNewClientInfo->viewDir, pNewClientInfo->x, pNewClientInfo->y, pNewClientInfo->hp);
		}
	}

	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pCI = iter->second;
		if (pCI->ID == newID)
		{
			continue;
		}
#ifdef LOG
		wprintf(L"PACKET_SC_CREATE_OHTER_CHARACTER\t Other To New %d -> %d\n", pCI->ID, newID);
#endif
		if (!SC_CREATE_OTHER_CHARACTER(g_sb, pCI->ID, newID, pCI->viewDir, pCI->x, pCI->y, pCI->hp))
		{
			return false;
		}

		if (pCI->action != dfPACKET_MOVE_DIR_NOMOVE)
		{
#ifdef LOG
			wprintf(L"NorifyMovingTo  %d - > %d\n", pCI->ID, newID);
#endif
			SC_MOVE_START(g_sb, pCI->ID, newID, pCI->action, pCI->x, pCI->y);
		}
	}
	return true;
}

void RecvProc(IN Session* pSession)
{
	int recvRet;
	int DeqRet;
	if (isDeletedSession(pSession->id))
	{
		return;
	}

	RingBuffer& recvRB = pSession->recvBuffer;
	int recvSize = recvRB.DirectEnqueueSize();
	if (recvSize == 0)
	{
		recvSize = recvRB.GetFreeSize();
	}
	recvRet = recv(pSession->clientSock, recvRB.GetWriteStartPtr(), recvSize, 0);
	if (recvRet == 0)
	{
#ifdef LOG
		wprintf(L"Session ID : %d, TCP CONNECTION END\n", pSession->id);
#endif
		disconnect(pSession->id);
		return;
	}
	else if (recvRet == SOCKET_ERROR)
	{
		int errCode = WSAGetLastError();
		if (errCode != WSAEWOULDBLOCK)
		{
#ifdef LOG
			wprintf(L"Session ID : %d, recv() error code : %d\n", pSession->id, errCode);
#endif
			disconnect(pSession->id);
		}
		return;
	}
#ifdef LOG
	wprintf(L"Session ID : %d, recv() -> recvRB size : %d\n", pSession->id, recvRet);
#endif
	recvRB.MoveRear(recvRet);

	ClientInfo* pCI = g_clientMap.find(pSession->id)->second;
	while (true)
	{
		Header header;
		DeqRet = recvRB.Dequeue((char*)&header, sizeof(header));
		if (DeqRet == 0)
		{
			return;
		}
		if (header.byCode != 0x89)
		{
#ifdef LOG
			wprintf(L"Session ID : %d\tDisconnected by INVALID HeaderCode Packet Received\n", pSession->id);
#endif
			disconnect(pSession->id);
			return;
		}
		// ����ȭ�����̹Ƿ� �Ź� �޽����� ó���ϱ� �������� rear_ == front_ == 0�̶�°��� ����
		if (g_sb.bufferSize_ < header.bySize)
		{
			g_sb.Resize();
		}
		int deqRet = recvRB.Dequeue(g_sb.GetBufferPtr(), header.bySize);
		if (deqRet == 0)
		{
			return;
		}
		g_sb.MoveWritePos(deqRet);
		PacketProc(pSession->id, header.byType, g_sb);
	}
}

Session::Session(IN SOCKET sock, IN int ID)
	:clientSock{ sock }, id{ ID }
{
}

void __forceinline DeleteBroadCasting(int removeId)
{
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		SC_DELETE_CHARACTER(g_sb, removeId, iter->second->ID);
	}
}

void deleteDisconnected()
{
	for (auto iter = g_disconnected_id_set.begin(); iter != g_disconnected_id_set.end();)
	{
		int id = *iter;
		auto SessionIter = g_sessionMap.find(id);
		auto ClientInfoIter = g_clientMap.find(id);
		closesocket(SessionIter->second->clientSock);
		delete SessionIter->second;
		delete ClientInfoIter->second;
		g_sessionMap.erase(id);
		g_clientMap.erase(id);
		iter = g_disconnected_id_set.erase(iter);
		//PACKET_SC_DELETE_CHARACTER PSDC;
		//MAKE_PACKET_SC_DELETE_CHATACTER(&PSDC, id);
#ifdef LOG
		wprintf(L"Session id : %d disconnected Broadcast To Other\n", id);
#endif
		DeleteBroadCasting(id);
		//EnqPacketBroadCast((char*)&PSDC, sizeof(PSDC), id);
		id_q.push(id);
	}
}

bool EnqNewRBForOtherCreate(IN Session* pNewUser)
{
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pCI = iter->second;
		if (isDeletedSession(pCI->ID) || pCI->ID == pNewUser->id)
			continue;


		PACKET_SC_CREATE_OTHER_CHARARCTER PSCOC;
		//MAKE_PACKET_SC_CREATE_OTHER_CHARACTER(&PSCOC, pCI);
		RingBuffer& sendRB = pNewUser->sendBuffer;
		int enqRet = sendRB.Enqueue((char*)&PSCOC, sizeof(PSCOC));
		if (enqRet == 0)
		{
			wprintf(L"Session ID : %d\tsendRingBuffer Full\t", pNewUser->id);
			disconnect(pNewUser->id);
			return false;
		}
#ifdef LOG
		wprintf(L"Session ID : %d  To Session ID : %d\tPACKET_SC_CREATE_OHTER_CHARACTER\t Ohter To New X : %d,Y : %d\n", pCI->ID, pNewUser->id, pCI->x, pCI->y);
		wprintf(L"sendRB Enqueue Size : %d\t sendRB FreeSize : %d\n", enqRet, sendRB.GetFreeSize());
#endif
	}
	return true;

}
void EnqPacketBroadCast(IN char* pPacket, IN const size_t packetSize, IN OPTIONAL const int pTargetIdToExcept)
{
	for (auto iter = g_sessionMap.begin(); iter != g_sessionMap.end(); ++iter)
	{
		Session* pSession = iter->second;
		if (pSession->id == pTargetIdToExcept)
			continue;
		EnqPacketUnicast(pSession->id, pPacket, packetSize);
	}
}

// �������� ���κп��� ��� Ŭ���̾�Ʈ���� ������ send
void SendProc(IN Session* pSession)
{
	// �����ؾ��ϴ� id��Ͽ� �����Ѵٸ� �׳� �ѱ��
	if (isDeletedSession(pSession->id))
	{
		return;
	}
	RingBuffer& sendRB = pSession->sendBuffer;
	int useSize = sendRB.GetUseSize();
	int directDeqSize = sendRB.DirectDequeueSize();
	while (useSize > 0)
	{
		// directDeqSize�� 0�̰ų� useSize < directDeqSize��� sendSize == useSize�̾�� �ϱ� �����̴�.
		int sendSize;
		if (useSize < directDeqSize || directDeqSize == 0)
		{
			sendSize = useSize;
		}
		else
		{
			sendSize = directDeqSize;
		}
		int sendRet = send(pSession->clientSock, sendRB.GetReadStartPtr(), sendSize, 0);
		if (sendRet == SOCKET_ERROR)
		{
			int errCode = WSAGetLastError();
			if (errCode != WSAEWOULDBLOCK)
			{
				wprintf(L"session ID : %d, send() func error code : %d #\n", pSession->id, errCode);
				disconnect(pSession->id);
			}
#ifdef LOG
			else
			{
				wprintf(L"session ID : %d send WSAEWOULDBLOCK #\n", pSession->id);
			}
#endif
			return;
		}
#ifdef LOG
		//wprintf(L"session ID : %d\tsend size : %d\tsendRB DirectDequeueSize : %d#\n", pSession->id, sendSize, sendRB.DirectDequeueSize());
#endif
		sendRB.MoveFront(sendSize);
		useSize = sendRB.GetUseSize();
		directDeqSize = sendRB.DirectDequeueSize();
	}
}

