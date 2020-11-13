#include "..\Engine\src\MicroEngine.c"
#include "system.c"

#include "gamezero.h"
#include "data.h"

#define MUI_ORIGIN_ID 1020

global u32 currentLevelIndex = 0;
global u32 nextLevelIndex = 0;

global Level levels[] = {
    LEVEL_MAP(00),
    LEVEL_MAP(01),
    LEVEL_MAP(02),
    LEVEL_MAP(03),
};

global GameResource gameResource;
global GameState gameState;

global MUI ui;
global MUI_Input uiInput;
global MicroECSWorld ecsWorld;
global Entity player;
global Vector2 gravity = {0.0f, 200.0f};
global char entityCountString[50] = "EntityCount";
