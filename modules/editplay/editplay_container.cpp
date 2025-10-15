#include "editplay_container.h"

#include "core/variant/variant.h"
#include "scene/main/viewport.h"

Size2 EditPlayContainer::get_minimum_size() const {
	if (stretch) {
		return Size2();
	}
	Size2 ms;

	if (!target_node) {
		return ms;
	}
	SubViewport* viewport = Object::cast_to<SubViewport>(target_node);
	if (!viewport) {
		return ms;
	}

	Size2 minsize = viewport->get_size();
	ms = ms.max(minsize);

	return ms;
}

void EditPlayContainer::set_stretch(bool p_enable) {
	if (stretch == p_enable) {
		return;
	}

	stretch = p_enable;
	recalc_force_viewport_sizes();
	update_minimum_size();
	// queue_sort();
	queue_redraw();
}

bool EditPlayContainer::is_stretch_enabled() const {
	return stretch;
}

void EditPlayContainer::set_stretch_shrink(int p_shrink) {
	ERR_FAIL_COND(p_shrink < 1);
	if (shrink == p_shrink) {
		return;
	}

	shrink = p_shrink;

	recalc_force_viewport_sizes();
	queue_redraw();
}

void EditPlayContainer::set_target_node(Variant new_target) {
	if (new_target.get_type() != Variant::OBJECT) {
		return;
	}
	if (!new_target.get_validated_object()) {
		return;
	}

	target_node = new_target;
}

Variant EditPlayContainer::get_target_node() const {
	return target_node;
}

void EditPlayContainer::recalc_force_viewport_sizes() {
	if (!stretch) {
		return;
	}

	if (!target_node.get_validated_object()) {
		return;
	}
	SubViewport* viewport = Object::cast_to<SubViewport>(target_node);
	if (!viewport) {
		return;
	}

	viewport->set_size_force(get_size() / shrink);
}

int EditPlayContainer::get_stretch_shrink() const {
	return shrink;
}

void EditPlayContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_RESIZED: {
			recalc_force_viewport_sizes();
		} break;

		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (!target_node.get_validated_object()) {
				return;
			}
			SubViewport* viewport = Object::cast_to<SubViewport>(target_node);
			if (!viewport) {
				return;
			}

			viewport->set_handle_input_locally(false); //do not handle input locally here
		} break;

		case NOTIFICATION_DRAW: {
			if (!target_node.get_validated_object()) {
				return;
			}
			SubViewport* viewport = Object::cast_to<SubViewport>(target_node);
			if (!viewport) {
				return;
			}

			if (stretch) {
				draw_texture_rect(viewport->get_texture(), Rect2(Vector2(), get_size()));
			} else {
				draw_texture_rect(viewport->get_texture(), Rect2(Vector2(), viewport->get_size()));
			}
		} break;

		case NOTIFICATION_FOCUS_ENTER: {
			// If focused, send InputEvent to the EditPlay before the Gui-Input stage.
			set_process_input(true);
			set_process_unhandled_input(false);
		} break;

		case NOTIFICATION_FOCUS_EXIT: {
			// A different Control has focus and should receive Gui-Input before the InputEvent is sent to the EditPlay.
			set_process_input(false);
			set_process_unhandled_input(true);
		} break;
	}
}

void EditPlayContainer::_notify_viewports(int p_notification) {
	if (!target_node.get_validated_object()) {
		return;
	}
	SubViewport* viewport = Object::cast_to<SubViewport>(target_node);
	if (!viewport) {
		return;
	}

	viewport->notification(p_notification);
}

void EditPlayContainer::call_input(const Ref<InputEvent> &p_event) {
	_propagate_nonpositional_event(p_event, true);
}

void EditPlayContainer::call_gui_input(const Ref<InputEvent> &p_event) {
	gui_input(p_event);
}

void EditPlayContainer::input(const Ref<InputEvent> &p_event) {
	_propagate_nonpositional_event(p_event, false);
}

void EditPlayContainer::unhandled_input(const Ref<InputEvent> &p_event) {
	_propagate_nonpositional_event(p_event, false);
}

void EditPlayContainer::_propagate_nonpositional_event(const Ref<InputEvent> &p_event, bool allow_gui_events) {
	ERR_FAIL_COND(p_event.is_null());

	if (!target_node.get_validated_object()) {
		return;
	}
	Viewport* viewport = Object::cast_to<Viewport>(target_node);
	if (!viewport || !viewport->is_inside_tree()) {
		return;
	}

	if (_is_propagated_in_gui_input(p_event) && !allow_gui_events) {
		return;
	}

	bool send;
	if (GDVIRTUAL_CALL(_propagate_input_event, p_event, send)) {
		if (!send) {
			return;
		}
	}

	_send_event_to_viewports(p_event);
}

void EditPlayContainer::gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	if (!target_node.get_validated_object()) {
		return;
	}
	SubViewport* viewport = Object::cast_to<SubViewport>(target_node);
	if (!viewport || !viewport->is_inside_tree()) {
		return;
	}

	if (!_is_propagated_in_gui_input(p_event)) {
		return;
	}

	bool send;
	if (GDVIRTUAL_CALL(_propagate_input_event, p_event, send)) {
		if (!send) {
			return;
		}
	}

	if (stretch && shrink > 1) {
		Transform2D xform;
		xform.scale(Vector2(1, 1) / shrink);
		_send_event_to_viewports(p_event->xformed_by(xform));
	} else {
		_send_event_to_viewports(p_event);
	}
}

void EditPlayContainer::_send_event_to_viewports(const Ref<InputEvent> &p_event) {
	if (!target_node.get_validated_object()) {
		return;
	}
	SubViewport* viewport = Object::cast_to<SubViewport>(target_node);
	if (!viewport) {
		return;
	}

	viewport->push_input(p_event);
}

bool EditPlayContainer::_is_propagated_in_gui_input(const Ref<InputEvent> &p_event) {
	// Propagation of events with a position property happen in gui_input
	// Propagation of other events happen in input
	if (Object::cast_to<InputEventMouse>(*p_event) || Object::cast_to<InputEventScreenDrag>(*p_event) || Object::cast_to<InputEventScreenTouch>(*p_event) || Object::cast_to<InputEventGesture>(*p_event)) {
		return true;
	}
	return false;
}

void EditPlayContainer::add_child_notify(Node *p_child) {
	queue_redraw();
}

void EditPlayContainer::remove_child_notify(Node *p_child) {
	queue_redraw();
}

PackedStringArray EditPlayContainer::get_configuration_warnings() const {
	PackedStringArray warnings = Node::get_configuration_warnings();

	if (target_node.is_null()) {
		warnings.push_back("You probably shouldn't use this.\nBut if you do, make sure the Target is set.");
	}

	if (get_default_cursor_shape() != Control::CURSOR_ARROW) {
		warnings.push_back(RTR("The default mouse cursor shape of EditPlayContainer has no effect.\nConsider leaving it at its initial value `CURSOR_ARROW`."));
	}

	return warnings;
}

void EditPlayContainer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_target_node", "new_target"), &EditPlayContainer::set_target_node);
	ClassDB::bind_method(D_METHOD("get_target_node"), &EditPlayContainer::get_target_node);
	ClassDB::bind_method(D_METHOD("call_input", "event"), &EditPlayContainer::call_input);
	ClassDB::bind_method(D_METHOD("call_gui_input", "event"), &EditPlayContainer::call_gui_input);

	GDVIRTUAL_BIND(_propagate_input_event, "event");
}

EditPlayContainer::EditPlayContainer() {
	set_process_unhandled_input(true);
	set_focus_mode(FOCUS_CLICK);
	target_node = Variant();
}
