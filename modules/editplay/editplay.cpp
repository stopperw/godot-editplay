/* editplay.cpp */

#include "editplay.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input_event.h"
#include "core/input/input_map.h"
#include "core/object/ref_counted.h"
#include "core/os/memory.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/packed_scene.h"
#ifdef TOOLS_ENABLED
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#endif

// E_EDITPLAY

EditPlay *EditPlay::singleton = nullptr;

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
	ep_scene = editor->new_scene();
	editor->set_edited_scene(world_viewport);

	// scene_tree = memnew(SceneTree);
	// root->get_parent()->remove_child(root);
	// scene_tree->get_root()->add_child(root);
	// scene_tree->set_current_scene(root);
	// scene_tree->get_root()->set_transient(true);
	// scene_tree->initialize();

	viewport = world_viewport;
	active_root = root;
#endif
}

void EditPlay::set_playing(bool is_playing) {
	playing = is_playing;
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
#endif
}

void EditPlay::ready() {
	if (active_root == nullptr) {
		return;
	}
	if (!active_root->is_inside_tree()) {
		return;
	}
	// active_root->notification(Node::NOTIFICATION_READY);
	// for (Variant obj : get_all_children(active_root)) {
	// 	Node* target = cast_to<Node>(obj.get_validated_object());
	// 	if (target == nullptr) {
	// 		continue;
	// 	}
	// 	target->notification(Node::NOTIFICATION_READY);
	// }
}

void EditPlay::process(double delta) {
	if (active_root == nullptr) {
		return;
	}
	if (!active_root->is_inside_tree()) {
		return;
	}
	// if (!scene_tree) {
	// 	return;
	// }
	// scene_tree->process(delta);

	// active_root->notification(Node::NOTIFICATION_PROCESS);
	// for (Variant obj : get_all_children(active_root)) {
	// 	Node* target = cast_to<Node>(obj.get_validated_object());
	// 	if (target == nullptr) {
	// 		continue;
	// 	}
	// 	target->notification(Node::NOTIFICATION_PROCESS);
	// }
}

void EditPlay::input(Ref<InputEvent> event) {
	if (active_root == nullptr) {
		return;
	}
	if (!active_root->is_inside_tree()) {
		return;
	}
	// active_root->notification(Node::NOTIFICATION_PROCESS);
	// for (Variant obj : get_all_children(active_root)) {
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
	for (Variant obj : get_all_children(viewport)) {
		Node *child = cast_to<Node>(obj.get_validated_object());
		child->set_editplay(false);
	}

	for (String action_name : created_actions) {
		InputMap::get_singleton()->erase_action(action_name);
	}

	EditorNode *editor = EditorNode::get_singleton();
	EditorData &data = editor->get_editor_data();
	if (viewport) {
		for (int i = 0; i < data.get_edited_scene_count(); i++) {
			EditorData::EditedScene scene = data.get_edited_scenes()[i];
			if (scene.root == viewport) {
				editor->close_scene(i);
			}
		}
	}

	created_actions.clear();
#endif
	active_root = nullptr;
	viewport = nullptr;
	ep_scene = -1;
	paused = false;
}

bool EditPlay::is_editplay() {
	return playing && !paused;
}

void EditPlay::init_autoload() {
#ifdef TOOLS_ENABLED
	if (!viewport) {
		return;
	}

	HashMap<StringName, ProjectSettings::AutoloadInfo> autoloads = ProjectSettings::get_singleton()->get_autoload_list();

	//first pass, add the constants so they exist before any script is loaded
	for (const KeyValue<StringName, ProjectSettings::AutoloadInfo> &E : autoloads) {
		const ProjectSettings::AutoloadInfo &info = E.value;

		if (info.is_singleton) {
			for (int i = 0; i < ScriptServer::get_language_count(); i++) {
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
				ScriptServer::get_language(i)->add_global_constant(info.name, n);
			}
		}
	}

	for (Node *E : to_add) {
		viewport->add_child(E);
	}
#endif
}

void EditPlay::deinit_autoload() {
	HashMap<StringName, ProjectSettings::AutoloadInfo> autoloads = ProjectSettings::get_singleton()->get_autoload_list();

	// removing created constants
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

void EditPlay::_bind_methods() {
	ClassDB::bind_method(D_METHOD("init", "world_viewport", "active_root"), &EditPlay::init);
	ClassDB::bind_method(D_METHOD("set_playing", "is_playing"), &EditPlay::set_playing);
	ClassDB::bind_method(D_METHOD("set_paused", "is_paused"), &EditPlay::set_paused);
	ClassDB::bind_method(D_METHOD("ready"), &EditPlay::ready);
	ClassDB::bind_method(D_METHOD("process", "delta"), &EditPlay::process);
	ClassDB::bind_method(D_METHOD("input", "event"), &EditPlay::input);
	ClassDB::bind_method(D_METHOD("engine_cleanup"), &EditPlay::engine_cleanup);
	ClassDB::bind_method(D_METHOD("is_editplay"), &EditPlay::is_editplay);
	ClassDB::bind_method(D_METHOD("init_autoload"), &EditPlay::init_autoload);
	ClassDB::bind_method(D_METHOD("deinit_autoload"), &EditPlay::deinit_autoload);
}

EditPlay::EditPlay() {
	singleton = this;
	scene_tree = nullptr;
	active_root = nullptr;
	viewport = nullptr;
	ep_scene = -1;
	playing = false;
	paused = false;
}

EditPlay::~EditPlay() {
}
