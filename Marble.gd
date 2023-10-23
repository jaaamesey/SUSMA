extends RigidBody3D
 
func _physics_process(delta):
	if global_position.y < -999 and !freeze:
		queue_free()
