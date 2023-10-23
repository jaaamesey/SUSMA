extends Panel

@export var help_suggestion_label: Control 

# Called when the node enters the scene tree for the first time.
func _ready():
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	if Input.is_action_just_pressed("help"):
		visible = !visible
		help_suggestion_label.visible = false
	if Input.is_key_pressed(KEY_ESCAPE):
		visible = false
	
	if get_viewport().use_xr:
		visible = false
		help_suggestion_label.visible = false
	
	get_tree().paused = visible

func _on_button_pressed():
	visible = false
