#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CUserCmd.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CBaseAnimState.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/CBaseWeapon.h"
#include "../FEATURES/Aimbot.h"
#include "../FEATURES/AntiAim.h"
#include "../UTILS/render.h"
#include "../SDK/CTrace.h"

namespace AAUTILS
{

	float randnum(float Min, float Max)
	{
		return ((float(rand()) / float(RAND_MAX)) * (Max - Min)) + Min;
	}

}


float get_curtime(SDK::CUserCmd* ucmd) {
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return 0;

	int g_tick = 0;
	SDK::CUserCmd* g_pLastCmd = nullptr;
	if (!g_pLastCmd || g_pLastCmd->hasbeenpredicted) {
		g_tick = local_player->GetTickBase();
	}
	else {
		++g_tick;
	}
	g_pLastCmd = ucmd;
	float curtime = g_tick * INTERFACES::Globals->interval_per_tick;
	return curtime;
}

bool next_lby_update(SDK::CUserCmd* m_pcmd) {
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (local_player) {
		static float next_lby_update_time;
		const float current_time = get_curtime(m_pcmd);
		local_update = next_lby_update_time;
		static bool in_air;
		static bool stop;
		stop = local_player->GetVelocity().Length2D() < 0.1 && local_player->GetFlags() & FL_ONGROUND;
		in_air = !(local_player->GetFlags() & FL_ONGROUND);
		if (stop || fake_walk || in_air) {
			if ((next_lby_update_time < current_time)) {
				next_lby_update_time = current_time + 1.1f;
				return true;
			}
		}
	}
	return false;
}

float fov_player(Vector ViewOffSet, Vector View, SDK::CBaseEntity* entity, int hitbox)
{
	// Anything past 180 degrees is just going to wrap around
	CONST FLOAT MaxDegrees = 180.0f;

	// Get local angles
	Vector Angles = View;

	// Get local view / eye position
	Vector Origin = ViewOffSet;

	// Create and intiialize vectors for calculations below
	Vector Delta(0, 0, 0);
	//Vector Origin(0, 0, 0);
	Vector Forward(0, 0, 0);

	// Convert angles to normalized directional forward vector
	MATH::AngleVectors(Angles, &Forward);

	Vector AimPos = aimbot->get_hitbox_pos(entity, hitbox); //pvs fix disabled

	MATH::VectorSubtract(AimPos, Origin, Delta);
	//Delta = AimPos - Origin;

	// Normalize our delta vector
	MATH::NormalizeNum(Delta, Delta);

	// Get dot product between delta position and directional forward vectors
	FLOAT DotProduct = Forward.Dot(Delta);

	// Time to calculate the field of view
	return (acos(DotProduct) * (MaxDegrees / M_PI));
}

int closest_to_crosshair()
{
	int index = -1;
	float lowest_fov = INT_MAX;

	SDK::CBaseEntity* local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return -1;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	Vector angles;
	INTERFACES::Engine->GetViewAngles(angles);

	for (int i = 1; i <= INTERFACES::Globals->maxclients; i++)
	{
		SDK::CBaseEntity *entity = INTERFACES::ClientEntityList->GetClientEntity(i);

		if (!entity || !entity->IsAlive() || entity->GetTeam() == local_player->GetTeam() || entity->GetIsDormant() || entity == local_player)
			continue;

		float fov = fov_player(local_position, angles, entity, 0);

		if (fov < lowest_fov)
		{
			lowest_fov = fov;
			index = i;
		}
	}

	return index;
}

void CAntiAim::freestand(SDK::CUserCmd* cmd, float yawangs)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	static float last_real;
	bool no_active = true;
	float bestrotation = 0.f;
	float highestthickness = 0.f;
	Vector besthead;

	auto leyepos = local_player->GetVecOrigin() + local_player->GetViewOffset();
	auto headpos = aimbot->get_hitbox_pos(local_player, 0);
	auto origin = local_player->GetAbsOrigin();

	auto checkWallThickness = [&](SDK::CBaseEntity* pPlayer, Vector newhead) -> float
	{
		Vector endpos1, endpos2;
		Vector eyepos = pPlayer->GetVecOrigin() + pPlayer->GetViewOffset();

		SDK::Ray_t ray;
		ray.Init(newhead, eyepos);

		SDK::CTraceFilterSkipTwoEntities filter(pPlayer, local_player);

		SDK::trace_t trace1, trace2;
		INTERFACES::Trace->TraceRay(ray, MASK_SHOT_BRUSHONLY, &filter, &trace1);

		if (trace1.DidHit())
			endpos1 = trace1.end;
		else
			return 0.f;

		ray.Init(eyepos, newhead);
		INTERFACES::Trace->TraceRay(ray, MASK_SHOT_BRUSHONLY, &filter, &trace2);

		if (trace2.DidHit())
			endpos2 = trace2.end;

		float add = newhead.Dist(eyepos) - leyepos.Dist(eyepos) + 3.f;
		return endpos1.Dist(endpos2) + add / 3;
	};

	int index = closest_to_crosshair();

	static SDK::CBaseEntity* entity;

	if (index != -1)
		entity = INTERFACES::ClientEntityList->GetClientEntity(index);

	float step = (2 * M_PI) / 18.f; // One PI = half a circle ( for stacker cause low iq :sunglasses: ), 28

	float radius = fabs(Vector(headpos - origin).Length2D());

	if (index == -1)
	{
		no_active = true;
	}
	else
	{
		for (float rotation = 0; rotation < (M_PI * 2.0); rotation += step)
		{
			Vector newhead(radius * cos(rotation) + leyepos.x, radius * sin(rotation) + leyepos.y, leyepos.z);

			float totalthickness = 0.f;

			no_active = false;

			totalthickness += checkWallThickness(entity, newhead);

			if (totalthickness > highestthickness)
			{
				highestthickness = totalthickness;
				bestrotation = rotation;
				besthead = newhead;
			}
		}
	}

	if (!GLOBAL::should_send_packet)
	{
		if (next_lby_update(cmd))
		{
			cmd->viewangles.y = last_real + SETTINGS::settings.antiaim.breaklby.delta;
		}
		else
		{
			if (no_active)
			{

			}
			else
				cmd->viewangles.y = RAD2DEG(bestrotation);

			last_real = cmd->viewangles.y;
		}
	}
}

void DoYawAA(SDK::CUserCmd* cmd, int index)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player)
		return;

	switch (index)
	{
	case 1:
		cmd->viewangles.y += 180;
		break;
	case 2:
	{
		float range = SETTINGS::settings.antiaim.jitterrange / 2;
		cmd->viewangles.y += 180 + AAUTILS::randnum(range, -range);
	}
	break;
	case 3:
	{
		static int Tick;
		Tick += SETTINGS::settings.antiaim.spinspeed + 1;

		cmd->viewangles.y += Tick;
	}
	break;
	case 4:
	{
		cmd->viewangles.y = local_player->GetLowerBodyYaw() - 120;
	}
	break;
	case 5:
	{
		switch (cmd->tick_count % 7)
		{
		case 0: cmd->viewangles.y = local_player->GetLowerBodyYaw() - 180; break;
		case 1: cmd->viewangles.y = local_player->GetLowerBodyYaw() + 180; break;
		case 2: cmd->viewangles.y += 180; break;
		case 3: cmd->viewangles.y -= 90; break;
		case 4: cmd->viewangles.y = local_player->GetLowerBodyYaw() - 90; break;
		case 5: cmd->viewangles.y = local_player->GetLowerBodyYaw() + 90; break;
		case 6: {
			if (GLOBAL::should_send_packet)
				cmd->viewangles.y = local_player->GetLowerBodyYaw() + 90;
			else cmd->viewangles.y = local_player->GetLowerBodyYaw() + 90;
		}
		break;
		}
		cmd->viewangles.y = local_player->GetLowerBodyYaw() + 90;
	}
	break;

	}
}


void CAntiAim::do_antiaim(SDK::CUserCmd* cmd)
{

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player)
		return;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));
	if (!weapon)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	if (local_player->GetImmunity())
		return;

	if (cmd->buttons & IN_USE)
		return;

	if (cmd->buttons & IN_ATTACK && aimbot->can_shoot(cmd))
		return;

	if (weapon->get_full_info()->WeaponType == 9)
		return;

	if (local_player->GetMoveType() == SDK::MOVETYPE_NOCLIP || local_player->GetMoveType() == SDK::MOVETYPE_LADDER)
		return;




	if (SETTINGS::settings.aa_bool)
	{

		if (local_player->GetVelocity().Length2D() < 0.1 && local_player->GetFlags() & FL_ONGROUND || fake_walk)
		{
			switch (SETTINGS::settings.antiaim.stand.pitch)
			{
			case 1:
				cmd->viewangles.x = 89.f;
				break;
			case 2:
				cmd->viewangles.x = AAUTILS::randnum(-89, 89);
				break;
			case 3:
				cmd->viewangles.x = 991;
				break;
			case 4:
				cmd->viewangles.x = 1080;
				break;
			}

			if (GLOBAL::should_send_packet)
				DoYawAA(cmd, SETTINGS::settings.antiaim.stand.fakeyaw);
			else
				DoYawAA(cmd, SETTINGS::settings.antiaim.stand.yaw);

		}
		else if (local_player->GetVelocity().Length2D() > 0.1 && local_player->GetFlags() & FL_ONGROUND)
		{
			switch (SETTINGS::settings.antiaim.move.pitch)
			{
			case 1:
				cmd->viewangles.x = 89.f;
				break;
			case 2:
				cmd->viewangles.x = AAUTILS::randnum(-89, 89);
				break;
			case 3:
				cmd->viewangles.x = 991;
				break;
			case 4:
				cmd->viewangles.x = 1080;
				break;
			}

			if (GLOBAL::should_send_packet)
				DoYawAA(cmd, SETTINGS::settings.antiaim.move.fakeyaw);
			else
				DoYawAA(cmd, SETTINGS::settings.antiaim.move.yaw);
		}
		else if (!(local_player->GetFlags() & FL_ONGROUND))
		{
			switch (SETTINGS::settings.antiaim.air.pitch)
			{
			case 1:
				cmd->viewangles.x = 89.f;
				break;
			case 2:
				cmd->viewangles.x = AAUTILS::randnum(-89, 89);
				break;
			case 3:
				cmd->viewangles.x = 991;
				break;
			case 4:
				cmd->viewangles.x = 1080;
				break;
			}

			if (GLOBAL::should_send_packet)
				DoYawAA(cmd, SETTINGS::settings.antiaim.air.fakeyaw);
			else
				DoYawAA(cmd, SETTINGS::settings.antiaim.air.yaw);
		}

		if (SETTINGS::settings.antiaim.breaklby.enable)
		{
			switch (SETTINGS::settings.antiaim.breaklby.type)
			{
			case 1:
			{
				static bool flip;
				if (GetKeyState(SETTINGS::settings.antiaim.breaklby.flipkey))
					flip = true;
				else
					flip = false;

				if (GLOBAL::should_send_packet)
				{
					if (flip)
						cmd->viewangles.y += 90;
					else
						cmd->viewangles.y -= 90;
				}
				else
				{
					if (next_lby_update(cmd)) {
						//GLOBAL::Msg("[getmem.es] lby update : %i    \n", local_update);
						cmd->viewangles.y += SETTINGS::settings.antiaim.breaklby.delta;
					}

					if (flip)
						cmd->viewangles.y -= 90;
					else
						cmd->viewangles.y += 90;
				}
			}
			break;
			case 2:
				freestand(cmd, 0);
				break;
			}
		}
	}
}

void CAntiAim::fix_movement(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	Vector real_viewangles;
	INTERFACES::Engine->GetViewAngles(real_viewangles);

	Vector vecMove(cmd->forwardmove, cmd->sidemove, cmd->upmove);
	float speed = sqrt(vecMove.x * vecMove.x + vecMove.y * vecMove.y);

	Vector angMove;
	MATH::VectorAngles(vecMove, angMove);

	float yaw = DEG2RAD(cmd->viewangles.y - real_viewangles.y + angMove.y);

	cmd->forwardmove = cos(yaw) * speed;
	cmd->sidemove = sin(yaw) * speed;

	cmd->viewangles = MATH::NormalizeAngle(cmd->viewangles);
}

CAntiAim* antiaim = new CAntiAim();