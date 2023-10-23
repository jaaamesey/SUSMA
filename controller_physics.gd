extends RigidBody3D


@onready var controller: XRController3D = get_parent()

# Called when the node enters the scene tree for the first time.
func _ready():
	connect("body_entered", func(body: Node):
		print("ENTER")
		controller.trigger_haptic_pulse("haptic", 0.1, 0.15, 0.1, 0)	
	)


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _physics_process(delta):
	global_position = get_parent().global_position
	global_rotation = get_parent().global_rotation
	if !get_viewport().use_xr:
		global_position = 10000000 * Vector3.ONE
