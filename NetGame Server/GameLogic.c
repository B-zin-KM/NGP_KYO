#include "Common.h"
#include "ServerFramework.h" 


void ProcessPlayerMove(int playerIndex, char* data) {
   
}

void ProcessPlayerAttack(int playerIndex, char* data) {

}

void UpdateEnemyAI() {

}

void CheckCollisions() {

}

bool CheckGameEndConditions() {
    return false;
}

void BroadcastPacket(char* packet, int size)
{
    EnterCriticalSection(&g_GameRoom.lock);
    {
        for (int i = 0; i < MAX_PLAYERS_PER_ROOM; i++)
        {
            if (g_GameRoom.players[i].isConnected == true)
            {
                send(g_GameRoom.players[i].socket, packet, size, 0);
            }
        }
    }
    LeaveCriticalSection(&g_GameRoom.lock);
}