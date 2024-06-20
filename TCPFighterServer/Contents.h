#pragma once
#include <WinSock2.h>
#include "SerializeBuffer.h"
#include "RingBuffer.h"
#include "Network.h"
constexpr int ERROR_RANGE = 50;
constexpr int MOVE_UNIT_X = 3;
constexpr int MOVE_UNIT_Y = 2;

constexpr char INIT_DIR = dfPACKET_MOVE_DIR_LL;
constexpr int INIT_POS_X = 300;
constexpr int INIT_POS_Y = 300;
constexpr int INIT_HP = 100;
constexpr int dfRANGE_MOVE_TOP = 50;
constexpr int dfRANGE_MOVE_LEFT = 10;
constexpr int dfRANGE_MOVE_RIGHT = 630;
constexpr int dfRANGE_MOVE_BOTTOM = 470;

//---------------------------------------------------------------
// 공격범위.
//---------------------------------------------------------------
constexpr int dfATTACK1_RANGE_X = 80;
constexpr int dfATTACK2_RANGE_X = 90;
constexpr int dfATTACK3_RANGE_X = 100;
constexpr int dfATTACK1_RANGE_Y = 10;
constexpr int dfATTACK2_RANGE_Y = 10;
constexpr int dfATTACK3_RANGE_Y = 20;

struct ClientInfo
{
	int ID;
	int hp;
	int x;
	int y;
	char viewDir;
	int action;
};

inline bool IsCollision_ATTACK1(IN ClientInfo* pAttackerCI, IN ClientInfo* pAttackedCI)
{
	int dX = abs(pAttackerCI->x - pAttackedCI->x);
	int dY = abs(pAttackerCI->y - pAttackedCI->y);
	bool isValidX = (dX <= dfATTACK1_RANGE_X) ? true : false;
	bool isValidY = (dY <= dfATTACK1_RANGE_Y) ? true : false;
	return isValidX && isValidY;
}

inline bool IsCollision_ATTACK2(IN ClientInfo* pAttackerCI, IN ClientInfo* pAttackedCI)
{
	int dX = abs(pAttackerCI->x - pAttackedCI->x);
	int dY = abs(pAttackerCI->y - pAttackedCI->y);
	bool isValidX = (dX <= dfATTACK2_RANGE_X) ? true : false;
	bool isValidY = (dY <= dfATTACK2_RANGE_Y) ? true : false;
	return isValidX && isValidY;
}

inline bool IsCollision_ATTACK3(IN ClientInfo* pAttackerCI, IN ClientInfo* pAttackedCI)
{
	int dX = abs(pAttackerCI->x - pAttackedCI->x);
	int dY = abs(pAttackerCI->y - pAttackedCI->y);
	bool isValidX = (dX <= dfATTACK3_RANGE_X) ? true : false;
	bool isValidY = (dY <= dfATTACK3_RANGE_Y) ? true : false;
	return isValidX && isValidY;
}

#define IN
#define OPTIONAL
#define OUT
void Update();
int MAKE_HEADER(IN SerializeBuffer& sb, IN BYTE packetSize, IN BYTE packetType);
bool SC_CREATE_MY_CHARACTER(IN SerializeBuffer& sb, int id, char direction, USHORT x, USHORT y, char hp);
bool SC_CREATE_OTHER_CHARACTER(SerializeBuffer& sb, int createId, int destId, char direction, USHORT x, USHORT y, char hp);
bool SC_DELETE_CHARACTER(SerializeBuffer& sb, int removeId, int destId);
bool SC_MOVE_START(SerializeBuffer& sb, int moveId, int destId, char moveDir, USHORT x, USHORT y);
bool SC_MOVE_STOP(SerializeBuffer& sb, int stopId, int destId, char viewDir, USHORT x, USHORT y);
bool SC_ATTACK1(SerializeBuffer& sb, int attackerId, int destId, char viewDir, USHORT x, USHORT y);
bool SC_ATTACK2(SerializeBuffer& sb, int attackerId, int destId, char viewDir, USHORT x, USHORT y);
bool SC_ATTACK3(SerializeBuffer& sb, int attackerId, int destId, char viewDir, USHORT x, USHORT y);
bool SC_DAMAGE(SerializeBuffer& sb, int destId, int attackerId, int victimId, char hpToReduce);
bool EnqPacketUnicast(IN const int id, IN char* pPacket, IN const size_t packetSize);
bool PacketProc(int id, BYTE packetType, SerializeBuffer& sb);
bool CS_MOVE_START(SerializeBuffer& sb, int moveId);
bool CS_MOVE_STOP(SerializeBuffer& sb, int stopId);
bool CS_ATTACK1(SerializeBuffer& sb, int attackerId);
bool CS_ATTACK2(SerializeBuffer& sb, int attackerId);
bool CS_ATTACK3(SerializeBuffer& sb, int attackerId);




