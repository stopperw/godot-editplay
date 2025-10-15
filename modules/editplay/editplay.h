/* editplay.h */

#ifndef EDITPLAY_H
#define EDITPLAY_H

#include "core/input/input_event.h"

class Node;

class EditPlay : public RefCounted {
	GDCLASS(EditPlay, RefCounted);

	static EditPlay *singleton;

	Node* viewport;
	Node* current_scene;
	Node* editor_current_scene;
	int ep_scene;
	bool playing;
	bool paused;
	bool freeze_cache;
	bool scene_close_stop;

	Vector<String> created_actions;

protected:
	static void _bind_methods();
	TypedArray<Node> get_all_children(Node* node) const;

public:
	_FORCE_INLINE_ static EditPlay *get_singleton() { return singleton; }

	void trigger_build() const;
	void init(Node* world_viewport, Node* current_scene);
	void set_playing(bool is_playing);
	void set_paused(bool is_paused);
	void set_current_scene(Node* current_scene);
	void ready();
	void process(double delta);
	void input(Ref<InputEvent> event);
	void engine_cleanup();
	bool is_editplay();
	void init_autoload(Node* world_viewport);
	void deinit_autoload();
	void fix_ownership(Node* node, Node *target_owner) const;
	void zap_cache();
	void trigger_scene_close_stop();

	Node* get_viewport();
	Node* get_current_scene();
	bool get_playing();
	bool is_paused();

	EditPlay();
	~EditPlay();
};

#endif // EDITPLAY_H
