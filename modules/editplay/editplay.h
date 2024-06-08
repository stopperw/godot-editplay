/* editplay.h */

#ifndef EDITPLAY_H
#define EDITPLAY_H

#include "scene/main/scene_tree.h"
class Node;

class EditPlay : public RefCounted {
	GDCLASS(EditPlay, RefCounted);

	static EditPlay *singleton;

	Node* viewport;
	Node* active_root;
	SceneTree* scene_tree;
	int ep_scene;
	bool playing;
	bool paused;

	Vector<String> created_actions;

protected:
	static void _bind_methods();
	TypedArray<Node> get_all_children(Node* node) const;

public:
	_FORCE_INLINE_ static EditPlay *get_singleton() { return singleton; }

	void init(Node* world_viewport, Node* root);
	void set_playing(bool is_playing);
	void set_paused(bool is_paused);
	void ready();
	void process(double delta);
	void input(Ref<InputEvent> event);
	void engine_cleanup();
	bool is_editplay();
	void init_autoload();
	void deinit_autoload();

	EditPlay();
	~EditPlay();
};

#endif // EDITPLAY_H
