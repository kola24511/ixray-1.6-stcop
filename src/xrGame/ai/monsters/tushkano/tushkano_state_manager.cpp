#include "stdafx.h"
#include "tushkano.h"
#include "tushkano_state_manager.h"

#include "../control_animation_base.h"
#include "../control_direction_base.h"
#include "../control_movement_base.h"
#include "../control_path_builder_base.h"

#include "../states/monster_state_rest.h"
#include "../states/monster_state_eat.h"
#include "../states/monster_state_attack.h"
#include "../states/monster_state_panic.h"
#include "../states/monster_state_hear_danger_sound.h"
#include "../states/monster_state_hitted.h"
#include "../states/monster_state_controlled.h"
#include "../states/monster_state_help_sound.h"

#include "../../../entitycondition.h"


CStateManagerTushkano::CStateManagerTushkano(CTushkanoBase* obj) : inherited(obj)
{
	m_pTushkano = smart_cast<CTushkanoBase*>(obj);

	add_state(eStateRest, xr_new<CStateMonsterRest>(obj));
	add_state(eStateAttack, xr_new<CStateMonsterAttack>(obj));
	add_state(eStateEat, xr_new<CStateMonsterEat>(obj));
	add_state(eStateHearDangerousSound, xr_new<CStateMonsterHearDangerousSound>(obj));
	add_state(eStatePanic, xr_new<CStateMonsterPanic>(obj));
	add_state(eStateHitted, xr_new<CStateMonsterHitted>(obj));
	add_state(eStateControlled, xr_new<CStateMonsterControlled>(obj));
	add_state(eStateHearHelpSound, xr_new<CStateMonsterHearHelpSound>(obj));
}


CStateManagerTushkano::~CStateManagerTushkano()
{
}

void CStateManagerTushkano::execute()
{
	u32 state_id = u32(-1);

	if (!m_pTushkano->is_under_control()) {
		const CEntityAlive* enemy	= object->EnemyMan.get_enemy();
//		const CEntityAlive* corpse	= 
			object->CorpseMan.get_corpse();

		if (enemy) {
			switch (object->EnemyMan.get_danger_type()) {
				case eStrong:	state_id = eStatePanic; break;
				case eWeak:		state_id = eStateAttack; break;
			}
		} else if (object->HitMemory.is_hit()) {
			state_id = eStateHitted;
		} else if (check_state(eStateHearHelpSound)) {
			state_id = eStateHearHelpSound;
		} else if (object->hear_interesting_sound || object->hear_dangerous_sound) {
			state_id = eStateHearDangerousSound;
		} else {
			if (can_eat())	state_id = eStateEat;
			else 			state_id = eStateRest;
		}
	} else state_id = eStateControlled;

	// установить текущее состояние
	select_state(state_id); 

	// выполнить текущее состояние
	get_state_current()->execute();

	prev_substate = current_substate;
}

