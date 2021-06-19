#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CUserCmd.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CTrace.h"
#include "../SDK/CBaseWeapon.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/ConVar.h"
#include "../FEATURES/AutoWall.h"
#include "../FEATURES/Aimbot.h"
#include "../FEATURES/Backtracking.h"

#define TICK_INTERVAL			( g_pGlobalVarsBase->interval_per_tick )

#define ROUND_TO_TICKS( t )		( TICK_INTERVAL * TIME_TO_TICKS( t ) )

template<class T> const T&
clamp(const T& x, const T& upper, const T& lower) { return min(upper, max(x, lower)); }
/*

std::deque<LagRecord> LagCompensation::m_PlayerTrack[MAX_PLAYERS];
std::deque<LagRecord> LagCompensation::m_PlayerTrack_Future[MAX_PLAYERS];

LagRecord LagCompensation::m_RestoreData; //[MAX_PLAYERS];
SDK::CBaseEntity* LagCompensation::m_pCurrentPlayer;

#define LC_NONE				0
#define LC_ALIVE			(1<<0)

#define LC_ORIGIN_CHANGED	(1<<8)
#define LC_ANGLES_CHANGED	(1<<9)
#define LC_SIZE_CHANGED		(1<<10)
#define LC_ANIMATION_CHANGED (1<<11)

static float GetAverageMovementCurve(int entIndex, SDK::CBaseEntity* ent)
{
	Vector p1 = Vector(0, 0, 0);
	Vector p2 = Vector(0, 0, 0);
	float ret = 0;
	int ticks = 0;

	for (size_t i = 0; i < LagCompensation::m_PlayerTrack[entIndex].size(); i++)
	{
		if (INTERFACES::Globals->curtime - LagCompensation::m_PlayerTrack[entIndex].at(i).m_flSimulationTime > ROTATION_CALC_TIME && i > 3)
			break;
		else if (i > 2)
		{

			float angle = 0;
			Vector a1, a2;
			MATH::VectorAngles(p1, a1);
			MATH::VectorAngles(p2, a2);
			if (a1.y < 0.0f)
				a1.y += 360.0f;
			if (a2.y < 0.0f)
				a2.y += 360.0f;
			angle = a2.y - a1.y;
			if (angle > 180.0f)
				angle -= 360.0f;
			ret += angle;
			ticks++;
		}

		p1 = p2;
		if (i > 0)
			p2 = (LagCompensation::m_PlayerTrack[entIndex].at(i).m_vecOrigin - LagCompensation::m_PlayerTrack[entIndex].at(i - 1).m_vecOrigin);
		p2.z = 0;
		p2.NormalizeInPlace();
	}

	ret /= max(1, ticks);

	return ret;
}*/

/*static Vector GetMovementDirection(int entIndex, SDK::CBaseEntity* ent)
{
	Vector ret = Vector(0, 0, 0);
	int ticks = 0;

	for (size_t i = 0; i < LagCompensation::m_PlayerTrack[entIndex].size(); i++)
		if (INTERFACES::Globals->curtime - LagCompensation::m_PlayerTrack[entIndex].at(i).m_flSimulationTime > DIRECTION_CALC_TIME && ticks++ > 1)
			break;
		else
			ret += LagCompensation::m_PlayerTrack[entIndex].at(i).m_vecVelocity;

	ret /= max(1, ticks);

	return ret.NormalizeInPlace();
}*/
/*
static float GetAverageAcceleration(int entIndex, SDK::CBaseEntity* ent)
{
	float vel = ent->GetVelocity().Length2D();
	float ret = 0;
	int ticks = 0;

	for (size_t i = 0; i < LagCompensation::m_PlayerTrack[entIndex].size(); i++)
		if (INTERFACES::Globals->curtime - LagCompensation::m_PlayerTrack[entIndex].at(i).m_flSimulationTime > ACCELERATION_CALC_TIME && ticks++ > 1)
			break;
		else
			ret += LagCompensation::m_PlayerTrack[entIndex].at(i).m_vecVelocity.Length2D() - vel;

	ret /= max(1, ticks);

	return ret;
}
*/
/*static void CreateCircles(Circle circles[5], int entIndex)
{
	size_t size = LagCompensation::m_PlayerTrack[entIndex].size();

	if (size >= 3)
	{
		size = 0;

		for (size = 0; size < LagCompensation::m_PlayerTrack[entIndex].size(); size++)
		{
			if (g_GlobalVars->curtime - LagCompensation::m_PlayerTrack[entIndex].at(size).m_flSimulationTime > ROTATION_CALC_TIME && size > 3)
				break;
		}

		circles[0] = Circle(LagCompensation::m_PlayerTrack[entIndex].at((size - 1) / 3).m_vecOrigin, LagCompensation::m_PlayerTrack[entIndex].at(2 * (size - 1) / 3).m_vecOrigin, LagCompensation::m_PlayerTrack[entIndex].at(size - 1).m_vecOrigin);
	}
	else if (size >= 1) {
		circles[0] = Circle(LagCompensation::m_PlayerTrack[entIndex].at(0).m_vecOrigin, 3604207201337.f, 1.f);
	}
}*/
/*
static float CalculateAverageSimtimeDelta(int entIndex)
{
	float ret = 0;

	for (size_t i = 1; i < LagCompensation::m_PlayerTrack[entIndex].size(); i++)
		ret += LagCompensation::m_PlayerTrack[entIndex].at(i - 1).m_flSimulationTime - LagCompensation::m_PlayerTrack[entIndex].at(i).m_flSimulationTime;

	ret /= max(1, (int)LagCompensation::m_PlayerTrack[entIndex].size());

	return ret;
}

static void CreateFutureTicks(int entIndex)
{
	SDK::CBaseEntity* ent = (SDK::CBaseEntity*)INTERFACES::ClientEntityList->GetClientEntity(entIndex);

	g_PlayerSettings[entIndex].averageSimTimeDelta = CalculateAverageSimtimeDelta(entIndex);

	SDK::INetChannelInfo* nci = INTERFACES::Engine->GetNetChannelInfo();

	if (nci->GetLatency(FLOW_OUTGOING) < g_PlayerSettings[entIndex].averageSimTimeDelta)
	{
		LagCompensation::m_PlayerTrack_Future[entIndex].clear();
		return;
	}

	//int predictAmount = ((nci->GetLatency(FLOW_OUTGOING) + nci->GetLatency(FLOW_INCOMING)) / INTERFACES::Globals->interval_per_tick) + 0.5f;

	//float predictionAngle = GetAverageMovementCurve(entIndex, ent);
	//float acceleration = GetAverageAcceleration(entIndex, ent);
	//Vector currentPosition = ent->GetVecOrigin();
	//Vector currentVelocity = ent->GetVelocity();

	//g_CVar->ConsoleDPrintf("Current velocity: %f %f %f\n", currentVelocity.x, currentVelocity.y, currentVelocity.z);

	SDK::CTraceWorldOnly filter;

	//LagRecord record = LagRecord(ent, nullptr);

	LagCompensation::m_PlayerTrack_Future[entIndex].clear();

	if (LagCompensation::m_PlayerTrack[entIndex].size() < 2)
		return;

	//Circle circles[5];

	//CreateCircles(circles, entIndex);

	Vector deltaVec = Vector(0, 0, 0);
	Vector origin = LagCompensation::m_PlayerTrack[entIndex].at(0).m_vecOrigin;

	/*for (int o = 0; o < 5; o++)
	{

		Vector movementDirection = LagCompensation::m_PlayerTrack[entIndex].at(0).m_vecVelocity;
		movementDirection.z = 0;
		//deltaVec = -circles[o].center + origin;
		//QAngle deltaAngle;
		//Math::VectorAngles(deltaVec, deltaAngle);
		//if (deltaAngle.yaw > 180.f)
		//	deltaAngle.yaw -= 360.f;
		//movementDirection = Math::RotateVectorYaw(Vector(0, 0, 0), deltaAngle.yaw + 180, movementDirection);
		movementDirection.NormalizeInPlace();

		bool isGrounded = ent->GetFlags() & FL_ONGROUND;

		for (int i = 0; i < predictAmount; i++)
		{
			float omega = currentVelocity.Length2D() * circles[o].iRadius;

			float predictionAngle = circles[o].direction * RAD2DEG(omega * g_GlobalVars->interval_per_tick);

			//g_CVar->ConsoleDPrintf("Predicted angle: %f Center: %f %f Rad: %f Vel: %f Accel: %f VelZ: %f \n", predictionAngle, circles[o].center.x, circles[o].center.y, circles[o].radius, currentVelocity.Length(), acceleration, currentVelocity.z);

			currentVelocity = Math::RotateVectorYaw(Vector(0, 0, 0), predictionAngle, currentVelocity);
			movementDirection = currentVelocity;
			movementDirection.z = 0;
			movementDirection.NormalizeInPlace();

			currentVelocity.x += acceleration * movementDirection.x * g_GlobalVars->interval_per_tick;
			currentVelocity.y += acceleration * movementDirection.y * g_GlobalVars->interval_per_tick;

			Movement::PlayerMove(ent, currentPosition, currentVelocity, isGrounded, true, g_GlobalVars->interval_per_tick);

			LagCompensation::m_PlayerTrack_Future[entIndex].push_back(LagRecord(LagRecord(ent, nullptr), currentPosition, (i + 1) * g_GlobalVars->interval_per_tick));
		}*.
		break;
	}*/
//}
/*
void LagCompensation::CreateMove(SDK::CUserCmd* cmd)
{
	if (!INTERFACES::Engine->IsInGame())
		return;

	for (int i = 1; i < 65; ++i)
	{
		SDK::CBaseEntity* ent = (SDK::CBaseEntity*)INTERFACES::ClientEntityList->GetClientEntity(i);
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

		if (!local_player)
			continue;
		if (!ent
			|| ent == local_player
			|| ent->GetIsDormant()
			|| !ent->GetHealth() > 0) {
			m_PlayerTrack[i].clear();
			continue;
		}

		SDK::player_info_t entityInformation;
		INTERFACES::Engine->GetPlayerInfo(i, &entityInformation);

		if (m_PlayerTrack[i].size() > 0 && m_PlayerTrack[i].at(0).m_flSimulationTime == ent->GetSimTime())
			continue;
		if (m_PlayerTrack[i].size() >= MAX_RECORDS - 1)
			m_PlayerTrack[i].pop_back();

		/*if (m_PlayerTrack[i].size() >= 1) {
			g_PlayerSettings[i].lastSimTimeDelta = ent->GetSimTime() - m_PlayerTrack[i].at(0).m_flSimulationTime;
			g_PlayerSettings[i].breakingLC = (ent->GetVecOrigin() - m_PlayerTrack[i].at(0).m_vecOrigin).Length2DSqr() > 4096;
		}*/

		/*m_PlayerTrack[i].push_front(LagRecord(ent, cmd));
		if (aim_type == 2)
			CreateFutureTicks(i);

		//if (g_PlayerSettings[i].breakingLC && m_PlayerTrack_Future[i].size() == 0)
			//g_PlayerSettings[i].breakingLC = false;
	}
}

void LagCompensation::StartLagCompensation(SDK::CBaseEntity* player)
{
	if (m_pCurrentPlayer)
		return;

	matrix3x4_t boneMatrix[MAXSTUDIOBONES];
	((SDK::CBaseAnimating*)player)->GetDirectBoneMatrix(boneMatrix);

	m_pCurrentPlayer = player;
	m_RestoreData = LagRecord(player, NULL);
	SDK::studiohdr_t *hdr = INTERFACES::ModelInfo->GetStudioModel(player->GetModel());
	if (!hdr)
		return;
	int size = hdr->numbones;
	for (int i = 0; i < size; i++)
		memcpy(m_RestoreData.boneMatrix + i, boneMatrix + i, sizeof(matrix3x4_t));
}

void LagCompensation::BacktrackPlayer(SDK::CBaseEntity* player, LagRecord record, bool setAngles)
{
	//Prevent the physics engine from culling the player
	if (setAngles)
	{
		player->SetAbsAngles(record.m_vecAngles);
		player->SetAbsOrigin(record.m_vecOrigin);
	}

	//Screw aiming at air with autowall especially, just tell the game what bonematrix to use
	((SDK::CBaseAnimating*)player)->SetBoneMatrix(record.boneMatrix);
}

void LagCompensation::EndLagCompensation(SDK::CBaseEntity* player)
{
	m_pCurrentPlayer = nullptr;

	BacktrackPlayer(player, m_RestoreData);
}*/

void CBacktrack::backtrack_player(SDK::CUserCmd* cmd)
{
	for (int i = 1; i < 65; ++i)
	{

		auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

		if (!entity)
			continue;

		if (!local_player)
			continue;

		bool is_local_player = entity == local_player;
		bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

		if (is_local_player)
			continue;

		if (is_teammate)
			continue;

		if (entity->GetHealth() <= 0)
			continue;

		if (local_player->GetHealth() <= 0)
			continue;

		if (entity->GetImmunity())
			continue;

		int index = entity->GetIndex();

		for (int i = 0; i < 20; i++)
		{
			backtrack_hitbox[index][i][cmd->command_number % 12] = aimbot->get_hitbox_pos(entity, i);
		}

		backtrack_simtime[index][cmd->command_number % 12] = entity->GetSimTime();

		for (int i = 0; i < 12; i++)
		{
			if (backtrack_simtime[index][i] != compensate[index][i])
			{
				if (i > 0 && i != 11)
				{
					oldest_tick[index] = i + 2;
				}
				else
				{
					oldest_tick[index] = 1;
				}
				compensate[index][i] = backtrack_simtime[index][i];
			}
		}
	}
}

inline Vector CBacktrack::angle_vector(Vector meme)
{
	auto sy = sin(meme.y / 180.f * static_cast<float>(M_PI));
	auto cy = cos(meme.y / 180.f * static_cast<float>(M_PI));

	auto sp = sin(meme.x / 180.f * static_cast<float>(M_PI));
	auto cp = cos(meme.x / 180.f* static_cast<float>(M_PI));

	return Vector(cp*cy, cp*sy, -sp);
}

inline float CBacktrack::point_to_line(Vector Point, Vector LineOrigin, Vector Dir)
{
	auto PointDir = Point - LineOrigin;

	auto TempOffset = PointDir.Dot(Dir) / (Dir.x*Dir.x + Dir.y*Dir.y + Dir.z*Dir.z);
	if (TempOffset < 0.000001f)
		return FLT_MAX;

	auto PerpendicularPoint = LineOrigin + (Dir * TempOffset);

	return (Point - PerpendicularPoint).Length();
}

void CBacktrack::run_legit(SDK::CUserCmd* cmd) //phook backtrack muahhahahahaaha
{
	int bestTargetIndex = -1;
	float bestFov = FLT_MAX;
	SDK::player_info_t info;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	for (int i = 1; i < 65; i++)
	{
		auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);

		if (!entity)
			continue;

		if (entity == local_player)
			continue;

		if (!INTERFACES::Engine->GetPlayerInfo(i, &info))
			continue;

		if (entity->GetIsDormant())
			continue;

		if (entity->GetTeam() == local_player->GetTeam())
			continue;

		if (!local_player->GetHealth() > 0)
			return;

		if (entity->GetHealth() > 0)
		{
			float simtime = entity->GetSimTime();
			Vector hitboxPos = aimbot->get_hitbox_pos(entity, 0);

			headPositions[i][cmd->command_number % 12] = legit_backtrackdata{ simtime, hitboxPos };
			Vector ViewDir = angle_vector(cmd->viewangles + (local_player->GetPunchAnglesRec() * 2.f));
			Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();
			float FOVDistance = point_to_line(hitboxPos, local_position, ViewDir);

			if (bestFov > FOVDistance)
			{
				bestFov = FOVDistance;
				bestTargetIndex = i;
			}
		}
	}

	float bestTargetSimTime;
	if (bestTargetIndex != -1)
	{
		float tempFloat = FLT_MAX;
		Vector ViewDir = angle_vector(cmd->viewangles + (local_player->GetPunchAnglesRec() * 2.f));
		Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

		for (int t = 0; t < 12; ++t)
		{
			float tempFOVDistance = point_to_line(headPositions[bestTargetIndex][t].hitboxPos, local_position, ViewDir);
			if (tempFloat > tempFOVDistance && headPositions[bestTargetIndex][t].simtime > local_player->GetSimTime() - 1)
			{
				tempFloat = tempFOVDistance;
				bestTargetSimTime = headPositions[bestTargetIndex][t].simtime;
			}
		}
		if (cmd->buttons & IN_ATTACK)
		{
			cmd->tick_count = TIME_TO_TICKS(bestTargetSimTime);
		}
	}
}
bool CBacktrack::IsValid(SDK::CBaseEntity * player)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return false;

	if (!player)
		return false;

	if (player->GetIsDormant() || player->GetHealth() == 0 || player->GetFlags() & FL_FROZEN)
		return false;

	if (player->GetTeam() == local_player->GetTeam())
		return false;

	if (player->GetClientClass()->m_ClassID != 35)
		return false;

	if (player == local_player)
		return false;

	if (player->GetImmunity())
		return false;

	return true;
}

CBacktrack* backtracking = new CBacktrack();
legit_backtrackdata headPositions[64][12];