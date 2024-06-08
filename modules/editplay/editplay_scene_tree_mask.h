/* editplay_scene_tree_mask.h */

#ifndef EDITPLAY_SCENE_TREE_MASK_H
#define EDITPLAY_SCENE_TREE_MASK_H

#include "core/string/print_string.h"
#include "scene/main/scene_tree.h"

class EditPlaySceneTreeMask : public SceneTree {
	GDCLASS(EditPlaySceneTreeMask, SceneTree);

public:
	_FORCE_INLINE_ Window *get_root() const { print_line("proxy called!"); return SceneTree::get_root(); };
};

#endif // EDITPLAY_SCENE_TREE_MASK_H
