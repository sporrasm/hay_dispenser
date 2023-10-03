width = 66.5;
height = 53;
depth = 13.5;
pin_hole_radius = 13 / 2;
mount_hole_radius = 1.7;
lock_hole_radius = 4;
hole_loc_x=37;
$fn=30;

diff_hole_y=24;
wall_thickness_x = 10;
wall_thickness_z = 10;
wall_thickness_y = 10;
extension_height = 20;

width_total = width + 2*wall_thickness_x;
depth_total = depth + 2*wall_thickness_z;
height_total = height + wall_thickness_y;

union() {
    difference() { // Main holder + mounting holes
        cube([width_total,depth_total,height_total]);
        translate([wall_thickness_x,wall_thickness_z,-1]) {
            cube([width, depth, height+1]);
        }
        translate([wall_thickness_x+hole_loc_x,-1,height_total-wall_thickness_y-6.5]) {
            rotate([0,90,90]) {
                cylinder(h=depth_total+2, r=mount_hole_radius);
            }
            rotate([0,90,90]) {
                translate([diff_hole_y,0,0]) {
                    cylinder(h=depth_total+2, r=mount_hole_radius);
                }
            }
        }
        center = (depth_total)/2;
        translate([width_total-wall_thickness_x-1,center,33+pin_hole_radius]) {
            rotate([0,90,0])
            cylinder(h = wall_thickness_z + 2, r = pin_hole_radius);
        }
    }
    difference() { // Extension + monting holes
        z = depth_total-wall_thickness_z;
        y = -extension_height;
        translate([0,z,y]) {
            cube([width_total,wall_thickness_z,extension_height+1]);

        }
        union() { // Mounting holes
            height_holes = 5;
            hole_left_x_loc = 19 + wall_thickness_x;
            hole_right_x_loc = 55 + wall_thickness_x;
            y = -extension_height + height_holes;
            z = depth_total + 1;
            translate([hole_left_x_loc,z,y]){
                rotate([90,0,0]) {
                    cylinder(h=wall_thickness_z+2, r=mount_hole_radius);
                }
            }
            translate([hole_right_x_loc,z,y]){
                rotate([90,0,0]) {
                    cylinder(h=wall_thickness_z+2, r=mount_hole_radius);
                }
            }
        }
        translate([hole_loc_x+wall_thickness_x,z,y]) {
            union() {
                translate([-lock_hole_radius,-1,-1]) {
                    cube([2*lock_hole_radius,wall_thickness_z+2,extension_height-lock_hole_radius+1]);
                }
                translate([0,wall_thickness_z+1,extension_height-lock_hole_radius]) {
                    rotate([90,0,0]) {
                        cylinder(h=wall_thickness_z+2,r=lock_hole_radius);
                    }
                }
             }
        
        }
    }
}

