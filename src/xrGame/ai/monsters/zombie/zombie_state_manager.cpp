#include "stdafx.h"
#include "zombie.h"
#include "zombie_state_manager.h"

#include "../control_animation_base.h"
#include "../control_direction_base.h"
#include "../control_movement_base.h"
#include "../control_path_builder_base.h"

#include "../states/monster_state_rest.h"
#include "../states/monster_state_attack.h"
#include "../states/monster_state_eat.h"
#include "../states/monster_state_hear_int_sound.h"
#include "zombie_state_attack_run.h"
#include "../../../entitycondition.h"
#include "../../../detail_path_manager.h"
#include "../states/monster_state_controlled.h"
#include "../states/monster_state_help_sound.h"

CStateManagerZombie::CStateManagerZombie(CZombie* obj) : inherited(obj)
{
	m_pZombie = smart_cast<CZombie*>(obj);

	add_state(eStateRest, xr_new<CStateMonsterRest>(obj));
	add_state(eStateAttack,
		xr_new<CStateMonsterAttack>(obj,
			xr_new<CStateZombieAttackRun>(obj),
			xr_new<CStateMonsterAttackMelee>(obj)
		)
	);
	add_state(eStateEat, xr_new<CStateMonsterEat>(obj));
	add_state(eStateHearInterestingSound, xr_new<CStateMonsterHearInterestingSound>(obj));
	add_state(eStateControlled, xr_new<CStateMonsterControlled>(obj));
	add_state(eStateHearHelpSound, xr_new<CStateMonsterHearHelpSound>(obj));
}


CStateManagerZombie::~CStateManagerZombie()
{
}

void CStateManagerZombie::execute()
{
	if (object->com_man().ta_is_active()) return;
	
	u32 state_id = u32(-1);
	
	if (!m_pZombie->is_under_control()) {
	
		const CEntityAlive* enemy	= object->EnemyMan.get_enemy();

		if (enemy) {
			state_id = eStateAttack;
		} else if (check_state(eStateHearHelpSound)) {
			state_id = eStateHearHelpSound;
		} else if (object->hear_interesting_sound || object->hear_dangerous_sound) {
			state_id = eStateHearInterestingSound;
		} else {
			if (can_eat())	state_id = eStateEat;
			else			state_id = eStateRest;
		}
	} else state_id = eStateControlled;

	// установить текущее состояние
	select_state(state_id); 

	// выполнить текущее состояние
	get_state_current()->execute();

	prev_substate = current_substate;
}

