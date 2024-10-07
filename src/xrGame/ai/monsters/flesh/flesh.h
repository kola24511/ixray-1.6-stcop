#pragma once
#include "../BaseMonster/base_monster.h"
#include "../controlled_entity.h"
#include "../../../../xrScripts/script_export_space.h"

class CFleshBase : public CBaseMonster,
				  public CControlledEntity {

	using		inherited = CBaseMonster					;
	using		CControlled = CControlledEntity	;

public:
	CFleshBase();
	virtual					~CFleshBase();
	
	virtual	void	Load					(LPCSTR section);
	virtual	BOOL	net_Spawn				(CSE_Abstract* DC);

	virtual	void	CheckSpecParams			(u32 spec_params);

	virtual bool	ability_can_drag		() {return true;}

	virtual	char*	get_monster_class_name () { return (char*)"flesh"; }


private:
	bool	ConeSphereIntersection	(Fvector ConeVertex, float ConeAngle, Fvector ConeDir, 
									Fvector SphereCenter, float SphereRadius);
	
	DECLARE_SCRIPT_REGISTER_FUNCTION
};