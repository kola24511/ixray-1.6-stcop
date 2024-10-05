#pragma once

#include "../state.h"

class CStateControlFire : public CState {
	typedef	CState		inherited;

	u32				m_time_started;
	u32				m_time_state_last_execute;

	CController* m_pController;

public:

	CStateControlFire(CBaseMonster* obj);
	virtual			~CStateControlFire	() {}

	virtual void	reinit					();
	virtual void	initialize				();
	virtual void	execute					();
	virtual void	finalize				();
	virtual void	critical_finalize		();
	virtual bool	check_completion		();
	virtual bool	check_start_conditions	();

};
