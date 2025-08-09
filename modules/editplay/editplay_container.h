#ifndef EDITPLAY_CONTAINER_H
#define EDITPLAY_CONTAINER_H

#include "core/variant/variant.h"
#include "scene/gui/container.h"
#include "scene/gui/control.h"

class EditPlayContainer : public Control {
	GDCLASS(EditPlayContainer, Control);

	bool stretch = true;
	int shrink = 1;
	Variant target_node;
	void _notify_viewports(int p_notification);
	bool _is_propagated_in_gui_input(const Ref<InputEvent> &p_event);
	void _send_event_to_viewports(const Ref<InputEvent> &p_event);
	void _propagate_nonpositional_event(const Ref<InputEvent> &p_event, bool allow_gui_events);

protected:
	void _notification(int p_what);
	static void _bind_methods();

	virtual void add_child_notify(Node *p_child) override;
	virtual void remove_child_notify(Node *p_child) override;

	GDVIRTUAL1RC(bool, _propagate_input_event, Ref<InputEvent>);

public:
	void set_stretch(bool p_enable);
	bool is_stretch_enabled() const;

	void set_target_node(Variant new_target);
	Variant get_target_node() const;

	virtual void call_input(const Ref<InputEvent> &p_event);
	virtual void call_gui_input(const Ref<InputEvent> &p_event);
	virtual void input(const Ref<InputEvent> &p_event) override;
	virtual void unhandled_input(const Ref<InputEvent> &p_event) override;
	virtual void gui_input(const Ref<InputEvent> &p_event) override;
	void set_stretch_shrink(int p_shrink);
	int get_stretch_shrink() const;
	void recalc_force_viewport_sizes();

	virtual Size2 get_minimum_size() const override;

	PackedStringArray get_configuration_warnings() const override;

	EditPlayContainer();
};

#endif // EDITPLAY_CONTAINER_H
