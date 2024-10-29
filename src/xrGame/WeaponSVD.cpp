#include "stdafx.h"
#include "pch_script.h"
#include "WeaponSvd.h"

CWeaponSVD::CWeaponSVD(void)
{}

CWeaponSVD::~CWeaponSVD(void)
{}

void CWeaponSVD::switch2_Fire()
{
	m_bFireSingleShot			= true;
	bWorking					= false;
	SetPending					(TRUE);
	m_iShotNum					= 0;
	m_bStopedAfterQueueFired	= false;

}

using namespace luabind;

#pragma optimize("s",on)
void CWeaponSVD::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CWeaponSVD,CGameObject>("CWeaponSVD")
			.def(constructor<>())
	];
}
