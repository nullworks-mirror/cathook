/*
 * prediction.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: nullifiedcat
 */

#include "prediction.h"

#include "common.h"
#include "sdk.h"

Vector SimpleLatencyPrediction(IClientEntity* ent, int hb) {
	//logging::Info("Simple prediction!");
	if (!ent) return Vector();
	Vector result;
	GetHitboxPosition(ent, hb, result);
	float latency = interfaces::engineClient->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) +
			interfaces::engineClient->GetNetChannelInfo()->GetLatency(FLOW_INCOMING);
	result += GetEntityValue<Vector>(ent, eoffsets.vVelocity) * latency;
	//logging::Info("Returning!");
	return result;
}

Vector ProjectilePrediction(IClientEntity* ent, int hb, float speed, float gravitymod) {
	if (!ent) return Vector();
	Vector result = ent->GetAbsOrigin();
	float dtg = DistanceToGround(result);
	GetHitboxPosition(ent, hb, result);
	Vector vel = GetEntityValue<Vector>(ent, eoffsets.vVelocity);
	// TODO ProjAim
	/*float tt = g_pLocalPlayer->v_Eye.DistTo(result) + interfaces::engineClient->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) +
		interfaces::engineClient->GetNetChannelInfo()->GetLatency(FLOW_INCOMING) - 0.20;
	float besttime = tt + 0.20;
	float mindelta = -1;
	for (int steps = 0; steps < 40; steps++, tt += 0.01) {
		Vector curpos = result;
		curpos += vel * tt;
		Vector projpos = v_Eye +
	}*/
	float dtt = g_pLocalPlayer->v_Eye.DistTo(result);
	float ttt = dtt / speed + interfaces::engineClient->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) +
		interfaces::engineClient->GetNetChannelInfo()->GetLatency(FLOW_INCOMING);
	float oz = result.z;
	int flags = GetEntityValue<int>(ent, eoffsets.iFlags);
	bool ground = (flags & (1 << 0));
	if (!ground) result.z -= ttt * ttt * 400;
	result += vel * ttt;
	if (oz - result.z > dtg) { result.z = oz - dtg; }
	result.z += (400 * ttt * ttt * gravitymod);
	// S = at^2/2 ; t = sqrt(2S/a)
	return result;
}

float DistanceToGround(Vector origin) {
	static trace_t* ground_trace = new trace_t();
	Ray_t ray;
	Vector endpos = origin;
	endpos.z -= 8192;
	ray.Init(origin, endpos);
	interfaces::trace->TraceRay(ray, 0x4200400B, trace::g_pFilterNoPlayer, ground_trace);
	return ground_trace->startpos.DistTo(ground_trace->endpos);
}
