#include "Contents.h"
#include "Network.h"
#include "SerializeBuffer.h"
#include <map>
#include <set>
#define IN
#define OUT
#define LOG

extern int g_IdToAlloc;
std::map<int, ClientInfo*> g_clientMap;
extern SerializeBuffer g_sb;
#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )

extern std::set<int> g_disconnected_id_set;
void InitClientInfo(OUT ClientInfo* pCI, IN int id)
{
	pCI->ID = id;
	pCI->hp = INIT_HP;
	pCI->x = INIT_POS_X;
	pCI->y = INIT_POS_Y;
	pCI->action = dfPACKET_MOVE_DIR_NOMOVE;
	pCI->viewDir = INIT_DIR;
#ifdef LOG
	wprintf(L"Create Character # SessionID : %d\tX:%d\tY:%d\n", id, INIT_POS_X, INIT_POS_Y);
#endif
}

void ClearClientInfo()
{
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end();)
	{
		delete iter->second;
		iter = g_clientMap.erase(iter);
	}
	g_clientMap.clear();
}
inline bool isDeletedSession(IN const int id)
{
	return g_disconnected_id_set.find(id) != g_disconnected_id_set.end();
}
inline void HandleMoving(IN ClientInfo* pCI, IN int moveDir)
{
	bool isNoX = false;
	bool isNoY = false;

	if (pCI->x - MOVE_UNIT_X <= dfRANGE_MOVE_LEFT || pCI->x + MOVE_UNIT_X >= dfRANGE_MOVE_RIGHT)
	{
		isNoX = true;
	}

	if (pCI->y - MOVE_UNIT_Y <= dfRANGE_MOVE_TOP || pCI->y + MOVE_UNIT_Y >= dfRANGE_MOVE_BOTTOM)
	{
		isNoY = true;
	}

	switch (moveDir)
	{
	case dfPACKET_MOVE_DIR_LL:
		if (!isNoX)
		{
			pCI->x -= MOVE_UNIT_X;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : LL\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		}
		break;
	case dfPACKET_MOVE_DIR_LU:
		if (!isNoX && !isNoY)
		{
			pCI->x -= MOVE_UNIT_X;
			pCI->y -= MOVE_UNIT_Y;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : LU\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		}
		break;
	case dfPACKET_MOVE_DIR_UU:
		if (!isNoY)
		{
			pCI->y -= MOVE_UNIT_Y;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : UU\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		}
		break;
	case dfPACKET_MOVE_DIR_RU:
		if (!isNoX && !isNoY)
		{
			pCI->x += MOVE_UNIT_X;
			pCI->y -= MOVE_UNIT_Y;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : RU\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
			break;
		}
	case dfPACKET_MOVE_DIR_RR:
		if (!isNoX)
		{
			pCI->x += MOVE_UNIT_X;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : RR\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		}
		break;
	case dfPACKET_MOVE_DIR_RD:
		if (!isNoX && !isNoY)
		{
			pCI->x += MOVE_UNIT_X;
			pCI->y += MOVE_UNIT_Y;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : RD\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		}
		break;
	case dfPACKET_MOVE_DIR_DD:
		if (!isNoY)
		{
			pCI->y += MOVE_UNIT_Y;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : DD\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		}
		break;
	case dfPACKET_MOVE_DIR_LD:
		if (!isNoX && !isNoY)
		{
			pCI->x -= MOVE_UNIT_X;
			pCI->y += MOVE_UNIT_Y;
#ifdef LOG
			wprintf(L"Session ID : %d #\tDIR : LD\tMove Simulate X : %d, Y : %d\n", pCI->ID, pCI->x, pCI->y);
#endif
		}
		break;
	default:
		__debugbreak();
		return;
	}
}
bool EnqPacketUnicast(IN const int id, IN char* pPacket, IN const size_t packetSize);


bool SC_CREATE_MY_CHARACTER(IN SerializeBuffer& sb, int id, char direction, USHORT x, USHORT y, char hp)
{
	constexpr int size = sizeof(id) + sizeof(direction) + sizeof(x) + sizeof(y) + sizeof(hp);
	int headerSize = MAKE_HEADER(sb, size, dfPACKET_SC_CREATE_MY_CHARACTER);
	sb << id << direction << x << y << hp;
	bool EnqRet = EnqPacketUnicast(id, sb.GetBufferPtr(), headerSize + size);
	sb.Clear();
	return EnqRet;
}
bool SC_CREATE_OTHER_CHARACTER(SerializeBuffer& sb, int createId, int destId, char direction, USHORT x, USHORT y, char hp)
{
	constexpr int size = sizeof(createId) + sizeof(direction) + sizeof(x) + sizeof(y) + sizeof(hp);
	int headerSize = MAKE_HEADER(sb, size, dfPACKET_SC_CREATE_OTHER_CHARACTER);
	sb << createId << direction << x << y << hp;
	bool EnqRet = EnqPacketUnicast(destId, sb.GetBufferPtr(), headerSize + size);
	sb.Clear();
	return EnqRet;
}

bool SC_DELETE_CHARACTER(SerializeBuffer& sb, int removeId, int destId)
{
	constexpr int size = sizeof(removeId);
	int headerSize = MAKE_HEADER(sb, size, dfPACKET_SC_DELETE_CHARACTER);
	sb << removeId;
	bool EnqRet = EnqPacketUnicast(destId, sb.GetBufferPtr(), headerSize + size);
	sb.Clear();
	return EnqRet;
}

bool SC_MOVE_START(SerializeBuffer& sb, int moveId, int destId, char moveDir, USHORT x, USHORT y)
{
	constexpr int size = sizeof(moveId) + sizeof(moveDir) + sizeof(x) + sizeof(y);
	int headerSize = MAKE_HEADER(sb, size, dfPACKET_SC_MOVE_START);
	sb << moveId << moveDir << x << y;
	bool EnqRet = EnqPacketUnicast(destId, sb.GetBufferPtr(), headerSize + size);
	sb.Clear();
	return EnqRet;
}

bool SC_MOVE_STOP(SerializeBuffer& sb, int stopId, int destId, char viewDir, USHORT x, USHORT y)
{
	constexpr int size = sizeof(stopId) + sizeof(viewDir) + sizeof(x) + sizeof(y);
	int headerSize = MAKE_HEADER(sb, size, dfPACKET_SC_MOVE_STOP);
	sb << stopId << viewDir << x << y;
	bool EnqRet = EnqPacketUnicast(destId, sb.GetBufferPtr(), headerSize + size);
	sb.Clear();
	return EnqRet;
}

bool SC_ATTACK1(SerializeBuffer& sb, int attackerId, int destId, char viewDir, USHORT x, USHORT y)
{
	constexpr int size = sizeof(attackerId) + sizeof(viewDir) + sizeof(x) + sizeof(y);
	int headerSize = MAKE_HEADER(sb, size, dfPACKET_SC_ATTACK1);
	sb << attackerId << viewDir << x << y;
	bool EnqRet = EnqPacketUnicast(destId, sb.GetBufferPtr(), headerSize + size);
	sb.Clear();
	return EnqRet;
}

bool SC_ATTACK2(SerializeBuffer& sb, int attackerId, int destId, char viewDir, USHORT x, USHORT y)
{
	constexpr int size = sizeof(attackerId) + sizeof(viewDir) + sizeof(x) + sizeof(y);
	int headerSize = MAKE_HEADER(sb, size, dfPACKET_SC_ATTACK2);
	sb << attackerId << viewDir << x << y;
	bool EnqRet = EnqPacketUnicast(destId, sb.GetBufferPtr(), headerSize + size);
	sb.Clear();
	return EnqRet;
}

bool SC_ATTACK3(SerializeBuffer& sb, int attackerId, int destId, char viewDir, USHORT x, USHORT y)
{
	constexpr int size = sizeof(attackerId) + sizeof(viewDir) + sizeof(x) + sizeof(y);
	int headerSize = MAKE_HEADER(sb, size, dfPACKET_SC_ATTACK3);
	sb << attackerId << viewDir << x << y;
	bool EnqRet = EnqPacketUnicast(destId, sb.GetBufferPtr(), headerSize + size);
	sb.Clear();
	return EnqRet;
}

bool SC_DAMAGE(SerializeBuffer& sb, int destId, int attackerId, int victimId, char hpToReduce)
{
	constexpr int size = sizeof(attackerId) + sizeof(victimId) + sizeof(hpToReduce);
	int headerSize = MAKE_HEADER(sb, size, dfPACKET_SC_DAMAGE);
	sb << attackerId << victimId << hpToReduce;
	bool EnqRet = EnqPacketUnicast(destId, sb.GetBufferPtr(), headerSize + size);
	sb.Clear();
	return EnqRet;
}

void Update()
{
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pCI = iter->second;
		if (isDeletedSession(pCI->ID))
		{
			continue;
		}

		if (pCI->action != dfPACKET_MOVE_DIR_NOMOVE)
		{
			HandleMoving(pCI, pCI->action);
		}

		if (pCI->hp <= 0)
		{
#ifdef LOG
			wprintf(L"Session ID : %d Die\t HP : %d\n", pCI->ID, pCI->hp);
#endif
			disconnect(pCI->ID);
		}
	}
	deleteDisconnected();
}


int MAKE_HEADER(IN SerializeBuffer& sb, IN BYTE packetSize, IN BYTE packetType)
{
	sb << (char)0x89 << (char)packetSize << (char)packetType;
	return sizeof((char)0x89) + sizeof(packetSize) + sizeof(packetType);
}


bool PacketProc(int id, BYTE packetType, SerializeBuffer& sb)
{
	switch (packetType)
	{
	case dfPACKET_CS_MOVE_START:
		return CS_MOVE_START(sb,id);
	case dfPACKET_CS_MOVE_STOP:
		return CS_MOVE_STOP(sb, id);
	case dfPACKET_CS_ATTACK1:
		return CS_ATTACK1(sb, id);
	case dfPACKET_CS_ATTACK2:
		return CS_ATTACK2(sb, id);
	case dfPACKET_CS_ATTACK3:
		return CS_ATTACK3(sb, id);
	default:
		return false;
	}
}

bool CS_MOVE_START(SerializeBuffer& sb, int moveId)
{
	char moveDir;
	USHORT x;
	USHORT y;
	sb >> moveDir >> x >> y;
	sb.Clear();

	ClientInfo* pStartCI = g_clientMap.find(moveId)->second;
	bool isNotValidPos = abs(pStartCI->x - x) > ERROR_RANGE || abs(pStartCI->y - y) > ERROR_RANGE;
	if (isNotValidPos)
	{
#ifdef LOG
		wprintf(L"Session ID : %d\t PROCESS_PACKET_CS_MOVE_START#\tMOVE_RANGE_ERROR - disconnected\n", moveId);
		wprintf(L"Server X : %d, Y : %d, Client X : %d, Y : %d\n", pStartCI->x, pStartCI->y, x, y);
#endif
		disconnect(moveId);
		return false;
	}
#ifdef LOG
	wprintf(L"Session ID : %d\tPACKET_CS_MOVE_START PROCESSING #\tMoveDir : %d\tX:%d\tY:%d\n", moveId, moveDir, x, y);
#endif
	switch (moveDir)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		pStartCI->viewDir = dfPACKET_MOVE_DIR_RR;
		break;
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LD:
		pStartCI->viewDir = dfPACKET_MOVE_DIR_LL;
		break;
	default:
		//__debugbreak();
		break;
	}
	pStartCI->x = x;
	pStartCI->y = y;
	pStartCI->action = moveDir;

	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pOtherCI = iter->second;
		if (pOtherCI != pStartCI)
		{
			SC_MOVE_START(sb, moveId, pOtherCI->ID, moveDir, x, y);
		}
	}
	return true;
}

bool CS_MOVE_STOP(SerializeBuffer& sb, int stopId)
{
	char viewDir;
	USHORT x;
	USHORT y;
	sb >> viewDir >> x >> y;
	sb.Clear();

	ClientInfo* pStopCI = g_clientMap.find(stopId)->second;
	bool isNotValidPos = abs(pStopCI->x - x) > ERROR_RANGE || abs(pStopCI->y - y) > ERROR_RANGE;
	if (isNotValidPos)
	{
#ifdef LOG
		wprintf(L"Session ID : %d\t PROCESS_PACKET_CS_MOVE_STOP#\tMOVE_RANGE_ERROR - disconnected\n", stopId);
		wprintf(L"Server X : %d, Y : %d, Client X : %d, Y : %d\n", pStopCI->x, pStopCI->y, x, y);
#endif
		disconnect(stopId);
		return false;
	}
#ifdef LOG
	wprintf(L"Session ID : %d\tPACKET_CS_MOVE_STOP PROCESSING #\tMoveDir : %d\tX:%d\tY:%d\n", stopId, viewDir, x, y);
#endif
	pStopCI->viewDir = viewDir;
	pStopCI->x = x;
	pStopCI->y = y;
	pStopCI->action = dfPACKET_MOVE_DIR_NOMOVE;
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pOtherCI = iter->second;
		if (pOtherCI->ID == stopId)
		{
			continue;
		}
#ifdef LOG
		wprintf(L"Session ID : %d -> Other\tPAKCET_SC_MOVE_STOP BroadCast\n", pOtherCI->ID);
#endif
		SC_MOVE_STOP(sb, stopId, pOtherCI->ID, viewDir, x, y);
	}
	return true;
}
void __forceinline DamageNotify(ClientInfo* pAttackerCI, ClientInfo* pVictimCI)
{
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pOtherCI = iter->second;
		//if (pOtherCI != pAttackerCI)
		//{
			SC_DAMAGE(g_sb, pOtherCI->ID, pAttackerCI->ID, pVictimCI->ID, pVictimCI->hp);
		//}
	}
}

bool CS_ATTACK1(SerializeBuffer& sb, int attackerId)
{
#ifdef LOG
	wprintf(L"Session ID : %d\t PROCESS_PACKET_CS_ATTACK1#\n", attackerId);
#endif
	char viewDir;
	USHORT x;
	USHORT y;
	sb >> viewDir >> x >> y;
	sb.Clear();

	ClientInfo* pAttackerCI = g_clientMap.find(attackerId)->second;
	if (!(pAttackerCI->x == x && pAttackerCI->y == y))
		__debugbreak();

#ifdef LOG
	wprintf(L"Session ID : %d -> Other \t PROCESS_PACKET_SC_ATTACK1 BroadCast#\n", attackerId);
#endif
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pOtherCI= iter->second;
		if (pOtherCI->ID != pAttackerCI->ID)
		{
			SC_ATTACK1(sb, pAttackerCI->ID, pOtherCI->ID, pAttackerCI->viewDir, pAttackerCI->x, pAttackerCI->y);
		}
	}

	constexpr int DAMAGE = 2;
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pOtherCI = iter->second;
		if (pOtherCI->ID != pAttackerCI->ID && IsCollision_ATTACK1(pAttackerCI,pOtherCI))
		{
#ifdef LOG
			wprintf(L"Session ID : %d -> Attacked %d\t PROCESS_PACKET_SC_DAMAGE#\n", attackerId, pOtherCI->ID);
#endif
			pOtherCI->hp -= DAMAGE;
			DamageNotify(pAttackerCI, pOtherCI);
		}
	}
	return true;
}

bool CS_ATTACK2(SerializeBuffer& sb, int attackerId)
{
#ifdef LOG
	wprintf(L"Session ID : %d\t PROCESS_PACKET_CS_ATTACK2#\n", attackerId);
#endif
	char viewDir;
	USHORT x;
	USHORT y;
	sb >> viewDir >> x >> y;
	sb.Clear();

	ClientInfo* pAttackerCI = g_clientMap.find(attackerId)->second;
	if (!(pAttackerCI->x == x && pAttackerCI->y == y))
		__debugbreak();

#ifdef LOG
	wprintf(L"Session ID : %d -> Other \t PROCESS_PACKET_SC_ATTACK2 BroadCast#\n", attackerId);
#endif
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pOtherCI = iter->second;
		if (pOtherCI->ID != pAttackerCI->ID)
		{
			SC_ATTACK2(sb, pAttackerCI->ID, pOtherCI->ID, pAttackerCI->viewDir, pAttackerCI->x, pAttackerCI->y);
		}
	}

	constexpr int DAMAGE = 2;
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pOtherCI = iter->second;
		if (pOtherCI->ID != pAttackerCI->ID && IsCollision_ATTACK2(pAttackerCI, pOtherCI))
		{
#ifdef LOG
			wprintf(L"Session ID : %d -> Attacked %d\t PROCESS_PACKET_SC_DAMAGE#\n", attackerId, pOtherCI->ID);
#endif
			pOtherCI->hp -= DAMAGE;
			DamageNotify(pAttackerCI, pOtherCI);
		}
	}
	return true;
}

bool CS_ATTACK3(SerializeBuffer& sb, int attackerId)
{
#ifdef LOG
	wprintf(L"Session ID : %d\t PROCESS_PACKET_CS_ATTACK3#\n", attackerId);
#endif
	char viewDir;
	USHORT x;
	USHORT y;
	sb >> viewDir >> x >> y;
	sb.Clear();

	ClientInfo* pAttackerCI = g_clientMap.find(attackerId)->second;
	if (!(pAttackerCI->x == x && pAttackerCI->y == y))
		__debugbreak();

#ifdef LOG
	wprintf(L"Session ID : %d -> Other \t PROCESS_PACKET_SC_ATTACK3 BroadCast#\n", attackerId);
#endif
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pOtherCI = iter->second;
		if (pOtherCI->ID != pAttackerCI->ID)
		{
			SC_ATTACK3(sb, pAttackerCI->ID, pOtherCI->ID, pAttackerCI->viewDir, pAttackerCI->x, pAttackerCI->y);
		}
	}

	constexpr int DAMAGE = 2;
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pOtherCI = iter->second;
		if (pOtherCI->ID != pAttackerCI->ID && IsCollision_ATTACK3(pAttackerCI, pOtherCI))
		{
#ifdef LOG
			wprintf(L"Session ID : %d -> Attacked %d\t PROCESS_PACKET_SC_DAMAGE#\n", attackerId, pOtherCI->ID);
#endif
			pOtherCI->hp -= DAMAGE;
			DamageNotify(pAttackerCI, pOtherCI);
		}
	}
	return true;
}
// 클라이언트의 접속을 끊는 함수들
void disconnect(IN int id)
{
	g_disconnected_id_set.insert(id);
#ifdef LOG
	wprintf(L"disconnect Session id : %d#\n", id);
#endif 
}
void NotifyMovingToNew(int newID)
{
	for (auto iter = g_clientMap.begin(); iter != g_clientMap.end(); ++iter)
	{
		ClientInfo* pCI = iter->second;
		if (isDeletedSession(pCI->ID) || pCI->action == dfPACKET_MOVE_DIR_NOMOVE || pCI->ID == newID)
			continue;

		wprintf(L"Session ID : %d\tNorifyMovingTo %d\n", pCI->ID, newID);
		PACKET_SC_MOVE_START PSMS;
		//MAKE_PACKET_SC_MOVE_START(&PSMS, pCI->ID, pCI->action, pCI->x, pCI->y);
		EnqPacketUnicast(newID,(char*)&PSMS, sizeof(PSMS));
	}
}

