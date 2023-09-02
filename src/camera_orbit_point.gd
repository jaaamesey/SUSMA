extends Node3D

@onready var x_rotator: Node3D = $XRotator
@onready var camera: Camera3D = $XRotator/Camera3D

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	pass # Replace with function body.

var _has_mouse_warped := false
func _input(event: InputEvent) -> void:
	var zoom_spd := 1.1
	if Input.is_action_just_pressed("zoom_in"):
		camera.position.z /= zoom_spd
	if Input.is_action_just_pressed("zoom_out"):
		camera.position.z *= zoom_spd
	camera.position.z = clamp(camera.position.z, 0.02, 10)
		
	var rotate_spd := 0.005
	if Input.is_action_pressed("rotate"):
		if event is InputEventMouseMotion:
			var warped_pos := (Vector2(
				fposmod(event.position.x, get_viewport().size.x),
				fposmod(event.position.y, get_viewport().size.y)
			))
			
			if warped_pos.distance_squared_to(event.position) > 0.01:
				Input.warp_mouse(warped_pos)
				_has_mouse_warped = true
			elif _has_mouse_warped:
				_has_mouse_warped = false
			else:
				var mouse_vel: Vector2 = event.relative
				x_rotator.rotate_x(-mouse_vel.y * rotate_spd)
				rotate_y(-mouse_vel.x * rotate_spd)

				
# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	if Input.is_action_pressed("rotate"):
		var spd := 1 * delta
		camera.position.x += (Input.get_action_strength("move_right") - Input.get_action_strength("move_left")) * spd
		camera.position.y += (Input.get_action_strength("move_up") - Input.get_action_strength("move_down")) * spd
