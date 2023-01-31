#include"hackLogic.h"
extern int gameWindowWidth ;
extern int gameWindowHeight ;
void HackLogic::Init()
{
	// This function will update and provide local Player and entity list pointer
	client = (uintptr_t) GetModuleHandleA( "Client.dll" );
	engine = (uintptr_t) GetModuleHandleA( "engine.dll" );
	entList = (EntList*) (client + dwEntityList);
	localEntity = entList->entlistObjPtrArray[0].entityObjPtr;

}

void HackLogic::Update()
{
	// this function continuously update ViewMatrix inside our Dll
	memcpy( &viewMatrix, (BYTE*) (client + dwViewMatrix), sizeof( viewMatrix ));
}

bool HackLogic::checkValidEnt( LocalEntity* ent )
{
	if (!ent)
		return false;
	if (ent == localEntity)
		return false;
	if (ent->iHealth <= 0)
		return false;
	if (ent->isDormant)
		return false;
	return true;
}

bool HackLogic::worldToScreen( Vec3 pos, Vec2& screen )
{
	Vec4 clipCoords;
	clipCoords.x = pos.x * viewMatrix[0] + pos.y * viewMatrix[1] + pos.z * viewMatrix[2] + viewMatrix[3];
	clipCoords.y = pos.x * viewMatrix[4] + pos.y * viewMatrix[5] + pos.z * viewMatrix[6] + viewMatrix[7];
	clipCoords.z = pos.x * viewMatrix[8] + pos.y * viewMatrix[9] + pos.z * viewMatrix[10] + viewMatrix[11];
	clipCoords.w = pos.x * viewMatrix[12] + pos.y * viewMatrix[13] + pos.z * viewMatrix[14] + viewMatrix[15];

	if (clipCoords.w < 0.1f)
		return false;
	Vec3 NDC;
	NDC.x = clipCoords.x / clipCoords.w;
	NDC.y = clipCoords.y / clipCoords.w;
	NDC.z = clipCoords.z / clipCoords.w;

	screen.x = (gameWindowWidth / 2 * NDC.x) + (NDC.x + gameWindowWidth / 2);
	screen.y = -(gameWindowHeight / 2 * NDC.y) + (NDC.y + gameWindowHeight / 2);
	return true;
}