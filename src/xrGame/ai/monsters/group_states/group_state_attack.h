#pragma once

#include "../state.h"
#include "../../../ai_debug.h"

class	CStateGroupAttack : public CState
{
public:
	CStateGroupAttack (CBaseMonster *obj);
	virtual				~CStateGroupAttack	();

	virtual void		initialize			();
	virtual	void		execute				();
	virtual void		setup_substates		();
	virtual	void		critical_finalize	();
	virtual	void		finalize		    ();
	virtual void		remove_links		(CObject* object);

protected:
	typedef CState		inherited;
	typedef CState*	state_ptr;

	const CEntityAlive* m_enemy;
	u32					m_time_next_run_away;
	u32					m_time_start_check_behinder;
	u32					m_time_start_behinder;
	float				m_delta_distance;
	u32					m_time_start_drive_out;
	bool				m_drive_out;

protected:
	bool				check_home_point	();
	bool				check_behinder		();
};

#include "group_state_attack_inline.h"
