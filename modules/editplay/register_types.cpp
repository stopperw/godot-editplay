/* register_types.cpp */

#include "register_types.h"

#include "core/object/class_db.h"
#include "editplay.h"
#include "editplay_container.h"

void initialize_editplay_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
			return;
	}
	ClassDB::register_class<EditPlay>();
	ClassDB::register_class<EditPlayContainer>();
}

void uninitialize_editplay_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
			return;
	}
}

