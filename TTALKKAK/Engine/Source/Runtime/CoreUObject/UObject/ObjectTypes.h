#pragma once


enum OBJECTS
{
    OBJ_ACTOR,
    OBJ_GAMEPLAYER,
    OBJ_CUBE,
    OBJ_SPHERE,
    OBJ_CAPSULE,
    OBJ_SPOTLIGHT,
    OBJ_POINTLIGHT,
    OBJ_DIRECTIONALLIGHT,
    OBJ_PARTICLE,
    OBJ_TEXT,
    OBJ_FOG,
    OBJ_CAR,
    OBJ_SKYSPHERE,
    OBJ_YEOUL,
    OBJ_SKELETAL,
    OBJ_CHARACTER,
    OBJ_END
};
enum ARROW_DIR
{
	AD_X,
	AD_Y,
	AD_Z,
	AD_END
};
enum ControlMode
{
	CM_TRANSLATION,
	CM_ROTATION,
	CM_SCALE,
	CM_END
};
enum CoordiMode
{
	CDM_WORLD,
	CDM_LOCAL,
	CDM_END
};
enum EPrimitiveColor
{
	RED_X,
	GREEN_Y,
	BLUE_Z,
	NONE,
	RED_X_ROT,
	GREEN_Y_ROT,
	BLUE_Z_ROT
};
