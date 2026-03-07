/* editplay.cpp */

#include "editplay.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input_map.h"
#include "core/object/ref_counted.h"
#include "core/os/memory.h"
#include "core/io/resource.h"
#include "scene/main/node.h"
#include "scene/main/window.h"
#include "scene/main/multiplayer_api.h"
#include "scene/resources/packed_scene.h"
#include "servers/audio/audio_server.h"
#ifdef TOOLS_ENABLED
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/settings/editor_settings.h"
#include "core/config/engine.h"
#endif

// E_EDITPLAY

EditPlay *EditPlay::singleton = nullptr;

void EditPlay::trigger_build() const {
#ifdef TOOLS_ENABLED
	EditorNode::get_singleton()->call_build();
#endif
}

void EditPlay::init(Node *world_viewport, Node *root) {
#ifdef TOOLS_ENABLED
	// Load InputMap
	List<PropertyInfo> pinfo;
	ProjectSettings::get_singleton()->get_property_list(&pinfo);

	for (const PropertyInfo &pi : pinfo) {
		if (!pi.name.begins_with("input/")) {
			continue;
		}

		String name = pi.name.substr(pi.name.find("/") + 1, pi.name.length());

		if (InputMap::get_singleton()->has_action(name)) {
			continue;
		}

		Dictionary action = GLOBAL_GET(pi.name);
		float deadzone = action.has("deadzone") ? (float)action["deadzone"] : 0.5f;
		Array events = action["events"];

		InputMap::get_singleton()->add_action(name, deadzone);
		created_actions.push_back(name);
		for (int i = 0; i < events.size(); i++) {
			Ref<InputEvent> event = events[i];
			if (event.is_null()) {
				continue;
			}
			InputMap::get_singleton()->action_add_event(name, event);
		}
	}

	world_viewport->set_editplay(true);
	for (Variant obj : get_all_children(world_viewport)) {
		Node *child = cast_to<Node>(obj.get_validated_object());
		child->set_editplay(true);
	}

	EditorNode *editor = EditorNode::get_singleton();
	playing = false;
	ep_scene = editor->new_scene();
	playing = true;
	editor->set_edited_scene(world_viewport);

	if (!editor_current_scene)
		editor_current_scene = world_viewport->get_tree()->get_current_scene();
	world_viewport->get_tree()->set_current_scene_unchecked(root);

	viewport = world_viewport;
	current_scene = root;
	set_playing(true);
#endif
}

void EditPlay::set_playing(bool is_playing) {
	playing = is_playing;
#ifdef TOOLS_ENABLED
	Engine::get_singleton()->set_editor_hint(!is_playing);
	AudioServer::get_singleton()->load_default_bus_layout();
#endif
}

void EditPlay::set_paused(bool is_paused) {
	paused = is_paused;
#ifdef TOOLS_ENABLED
	if (viewport == nullptr) {
		return;
	}
	if (!viewport->is_inside_tree()) {
		return;
	}
	for (Variant obj : get_all_children(viewport)) {
		Node *child = cast_to<Node>(obj.get_validated_object());
		child->set_editplay(!is_paused);
	}
	if (is_paused)
		viewport->propagate_notification(Node::NOTIFICATION_PAUSED);
	else
		viewport->propagate_notification(Node::NOTIFICATION_UNPAUSED);
#endif
}

void EditPlay::set_current_scene(Node* root) {
	current_scene = root;
}

void EditPlay::ready() {
	if (current_scene == nullptr) {
		return;
	}
	if (!current_scene->is_inside_tree()) {
		return;
	}
	// current_scene->notification(Node::NOTIFICATION_READY);
	// for (Variant obj : get_all_children(current_scene)) {
	// 	Node* target = cast_to<Node>(obj.get_validated_object());
	// 	if (target == nullptr) {
	// 		continue;
	// 	}
	// 	target->notification(Node::NOTIFICATION_READY);
	// }
}

void EditPlay::process(double delta) {
	if (current_scene == nullptr) {
		return;
	}
	if (!current_scene->is_inside_tree()) {
		return;
	}

	// current_scene->notification(Node::NOTIFICATION_PROCESS);
	// for (Variant obj : get_all_children(current_scene)) {
	// 	Node* target = cast_to<Node>(obj.get_validated_object());
	// 	if (target == nullptr) {
	// 		continue;
	// 	}
	// 	target->notification(Node::NOTIFICATION_PROCESS);
	// }
}

void EditPlay::input(Ref<InputEvent> event) {
	if (current_scene == nullptr) {
		return;
	}
	if (!current_scene->is_inside_tree()) {
		return;
	}
	// current_scene->notification(Node::NOTIFICATION_PROCESS);
	// for (Variant obj : get_all_children(current_scene)) {
	// 	Node* target = cast_to<Node>(obj.get_validated_object());
	// 	if (target == nullptr) {
	// 		continue;
	// 	}
	// 	target->notification(Node::NOTIFICATION_PROCESS);
	// }
}

void EditPlay::engine_cleanup() {
#ifdef TOOLS_ENABLED
	if (!viewport) {
		return;
	}

	if (!viewport->is_inside_tree()) {
		return;
	}

	Ref<MultiplayerAPI> api = viewport->get_tree()->get_multiplayer();
	if (!api.is_null()) {
		api->set_multiplayer_peer(nullptr);
	}

	if (editor_current_scene)
		viewport->get_tree()->set_current_scene_unchecked(editor_current_scene);

	for (Variant obj : get_all_children(viewport)) {
		Node *child = cast_to<Node>(obj.get_validated_object());
		child->set_editplay(false);
	}

	for (String action_name : created_actions) {
		InputMap::get_singleton()->erase_action(action_name);
	}

	EditorNode *editor = EditorNode::get_singleton();
	EditorData &data = editor->get_editor_data();
	for (int i = 0; i < data.get_edited_scene_count(); i++) {
		EditorData::EditedScene scene = data.get_edited_scenes()[i];
		if (scene.root == viewport && !scene_close_stop) {
			editor->close_scene(i);
		}
	}

	created_actions.clear();
#endif
	current_scene = nullptr;
	viewport = nullptr;
	editor_current_scene = nullptr;
	ep_scene = -1;
	set_playing(false);
	paused = false;
	freeze_cache = false;
	scene_close_stop = false;
}

void EditPlay::init_autoload(Node *world_viewport) {
#ifdef TOOLS_ENABLED
	if (!world_viewport) {
		return;
	}

	// copied from engine's autoload init code
	HashMap<StringName, ProjectSettings::AutoloadInfo> autoloads = ProjectSettings::get_singleton()->get_autoload_list();

	//first pass, add the constants so they exist before any script is loaded
	for (const KeyValue<StringName, ProjectSettings::AutoloadInfo> &E : autoloads) {
		const ProjectSettings::AutoloadInfo &info = E.value;

		if (info.is_singleton) {
			for (int i = 0; i < ScriptServer::get_language_count(); i++) {
				ScriptServer::get_language(i)->add_named_global_constant(info.name, Variant());
				ScriptServer::get_language(i)->add_global_constant(info.name, Variant());
			}
		}
	}

	//second pass, load into global constants
	List<Node *> to_add;
	for (const KeyValue<StringName, ProjectSettings::AutoloadInfo> &E : autoloads) {
		const ProjectSettings::AutoloadInfo &info = E.value;

		Node *n = nullptr;
		if (ResourceLoader::get_resource_type(info.path) == "PackedScene") {
			// Cache the scene reference before loading it (for cyclic references)
			Ref<PackedScene> scn;
			scn.instantiate();
			scn->set_path(info.path);
			scn->reload_from_file();
			ERR_CONTINUE_MSG(!scn.is_valid(), vformat("Failed to instantiate an autoload, can't load from path: %s.", info.path));

			if (scn.is_valid()) {
				n = scn->instantiate();
			}
		} else {
			Ref<Resource> res = ResourceLoader::load(info.path);
			ERR_CONTINUE_MSG(res.is_null(), vformat("Failed to instantiate an autoload, can't load from path: %s.", info.path));

			Ref<Script> script_res = res;
			if (script_res.is_valid()) {
				StringName ibt = script_res->get_instance_base_type();
				bool valid_type = ClassDB::is_parent_class(ibt, "Node");
				ERR_CONTINUE_MSG(!valid_type, vformat("Failed to instantiate an autoload, script '%s' does not inherit from 'Node'.", info.path));

				Object *obj = ClassDB::instantiate(ibt);
				ERR_CONTINUE_MSG(!obj, vformat("Failed to instantiate an autoload, cannot instantiate '%s'.", ibt));

				n = Object::cast_to<Node>(obj);
				n->set_script(script_res);
			}
		}

		ERR_CONTINUE_MSG(!n, vformat("Failed to instantiate an autoload, path is not pointing to a scene or a script: %s.", info.path));
		n->set_name(info.name);

		//defer so references are all valid on _ready()
		to_add.push_back(n);

		if (info.is_singleton) {
			for (int i = 0; i < ScriptServer::get_language_count(); i++) {
				ScriptServer::get_language(i)->add_named_global_constant(info.name, n);
				ScriptServer::get_language(i)->add_global_constant(info.name, n);
			}
		}
	}

	for (Node *E : to_add) {
		for (Variant obj : get_all_children(E)) {
			Node *child = cast_to<Node>(obj.get_validated_object());
			child->set_editplay(true);
		}
		E->set_editplay(true);
		world_viewport->add_child(E);
	}
#endif
}

void EditPlay::deinit_autoload() {
	HashMap<StringName, ProjectSettings::AutoloadInfo> autoloads = ProjectSettings::get_singleton()->get_autoload_list();

	// removing created constants
	// or not really, because editor loads some singletons too.
	// for (const KeyValue<StringName, ProjectSettings::AutoloadInfo> &E : autoloads) {
	// 	const ProjectSettings::AutoloadInfo &info = E.value;
	//
	// 	if (info.is_singleton) {
	// 		for (int i = 0; i < ScriptServer::get_language_count(); i++) {
	// 			ScriptServer::get_language(i)->remove_named_global_constant(info.name);
	// 		}
	// 	}
	// }
}

void EditPlay::fix_ownership(Node *node, Node *target_owner) const {
#ifdef TOOLS_ENABLED
	bool unwrap = EDITOR_GET("editplay/make_all_children_editable");

	for (Variant obj : get_all_children(node)) {
		Node *child = cast_to<Node>(obj.get_validated_object());
		if (child->get_owner() == node || unwrap)
			child->set_owner(target_owner);
	}
#endif
}

void EditPlay::zap_cache() {
#ifdef TOOLS_ENABLED
	ResourceCache::editplay_clear();
#endif
}

Node* EditPlay::get_viewport() {
	return viewport;
}

Node* EditPlay::get_current_scene() {
	return current_scene;
}

bool EditPlay::get_playing() {
	return playing;
}

bool EditPlay::is_paused() {
	return paused;
}

TypedArray<Node> EditPlay::get_all_children(Node *node) const {
	TypedArray<Node> queue;
	queue.push_back(node);
	TypedArray<Node> children;
	while (!queue.is_empty()) {
		Node *target = cast_to<Node>(queue.pop_back().get_validated_object());
		TypedArray<Node> targetChildren = target->get_children();
		children.append_array(targetChildren);
		queue.append_array(targetChildren);
	}
	return children;
}

void EditPlay::trigger_scene_close_stop() {
	scene_close_stop = true;
	emit_signal(SNAME("stop_triggered"));
}

void EditPlay::_bind_methods() {
	ClassDB::bind_method(D_METHOD("trigger_build"), &EditPlay::trigger_build);
	ClassDB::bind_method(D_METHOD("init", "world_viewport", "current_scene"), &EditPlay::init);
	ClassDB::bind_method(D_METHOD("set_playing", "is_playing"), &EditPlay::set_playing);
	ClassDB::bind_method(D_METHOD("set_paused", "is_paused"), &EditPlay::set_paused);
	ClassDB::bind_method(D_METHOD("ready"), &EditPlay::ready);
	ClassDB::bind_method(D_METHOD("process", "delta"), &EditPlay::process);
	ClassDB::bind_method(D_METHOD("input", "event"), &EditPlay::input);
	ClassDB::bind_method(D_METHOD("engine_cleanup"), &EditPlay::engine_cleanup);
	ClassDB::bind_method(D_METHOD("init_autoload", "world_viewport"), &EditPlay::init_autoload);
	ClassDB::bind_method(D_METHOD("deinit_autoload"), &EditPlay::deinit_autoload);
	ClassDB::bind_method(D_METHOD("fix_ownership", "node", "target_owner"), &EditPlay::fix_ownership);
	ClassDB::bind_method(D_METHOD("zap_cache"), &EditPlay::zap_cache);

	ClassDB::bind_method(D_METHOD("get_viewport"), &EditPlay::get_viewport);
	ClassDB::bind_method(D_METHOD("get_current_scene"), &EditPlay::get_current_scene);
	ClassDB::bind_method(D_METHOD("get_playing"), &EditPlay::get_playing);
	ClassDB::bind_method(D_METHOD("is_paused"), &EditPlay::is_paused);

	ADD_SIGNAL(MethodInfo("stop_triggered"));
}

EditPlay::EditPlay() {
	singleton = this;
	viewport = nullptr;
	current_scene = nullptr;
	editor_current_scene = nullptr;
	ep_scene = -1;
	playing = false;
	paused = false;
	freeze_cache = false;
	scene_close_stop = false;
}

EditPlay::~EditPlay() {
}
