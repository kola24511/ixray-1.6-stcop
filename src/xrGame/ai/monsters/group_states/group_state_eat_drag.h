#pragma once

#include "../state.h"

class CustomDog;

class CStateGroupDrag : public CState {
	typedef CState	inherited;

	Fvector				m_cover_position;
	u32					m_cover_vertex_id;
	
	bool				m_failed;
	Fvector				m_corpse_start_position;

	CustomDog*			m_pDog;

public:
						CStateGroupDrag		(CBaseMonster* object);
	virtual				~CStateGroupDrag		();

	virtual void		initialize				();
	virtual	void		execute					();
	virtual void		finalize				();
	virtual void		critical_finalize		();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}

	virtual bool		check_completion		();
};
