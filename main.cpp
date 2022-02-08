#include <iostream>


#include "common/bmp.h"
#include "common/hittable_list.h"
#include "common/sphere.h"
#include "common/rtweekend.h"
#include "common/camera.h"
#include "common/material.h"


/* bool hit_sphere(const point3& center, double radius, const ray& r){
    vec3 oc = r.origin() - center;
    auto a = dot(r.direction(), r.direction());
    auto b = 2.0 * dot(oc,r.direction());
    auto c = dot(oc,oc) - radius*radius;
    auto discriminant = b*b - 4*a*c;
    if(discriminant<0){
        return -1;
    } else {
        return (( -b - sqrt(discriminant)) / (2.0*1));
    }

} */

/* double hit_sphere(const point3& center, double radius, const ray& r){
    vec3 oc = r.origin() - center;
    auto a = dot(r.direction(),r.direction());
    auto b = dot(oc,r.direction()); // original b = 2.0*...
    auto c = dot(oc,oc) - radius*radius;
    auto discriminant = b*b - 4 * a * c;
    if(discriminant < 0 ){
        return -1.0;
    } else {
        return (-b - sqrt(discriminant) ) / (2.0*a);
    }
} */


color ray_color(const ray& r, const hittable& world, int depth){
    hit_record rec;
    if(depth<=0){
        return color(0,0,0);
    }
    if(world.hit(r,0.001,infinity,rec)) {
        ray scattered;
        color attenuation;
        if(rec.mat_ptr->scatter(r,rec,attenuation,scattered))
            return attenuation * ray_color(scattered,world,depth-1);
        return color(0,0,0);
        /* point3 target = rec.p + rec.normal + random_unit_vector();
        return 0.5 * ray_color(ray(rec.p, target - rec.p), world,depth-1); */
    }
    vec3 unit_direction = unit_vector(r.direction());
    auto t = 0.5*(unit_direction.y() + 1.0);
    return (1.0-t)*color(1.0,1.0,1.0) + t*color(0.5,0.7,1.0);
}




// (A) * (A + B) = A*A + A*B
// A * (B + C) = A*B + A*C
// (A-B) * (C-D) = (A-B)*C - (A-B)*D = A*C - B*C - A*D + B*D
// (A + tB - C) * (A + tB - C)
// = (A + tB - C)*tB + (A + tB - C)*(A-C) 
// = tt(B*B) + (A-C)*tB + tB*(A-C) + (A-C)*(A-C)
// = ttBB + 2tB*(A-C) + (A-C)*(A-C)

hittable_list random_scene() {
    hittable_list world;

    auto ground_material = make_shared<lambertian>(color(0.5,0.5,0.5));
    world.add(make_shared<sphere>(point3(0,-1000,0),1000,ground_material));

   /*  for(int a = -11; a < 11; a++){
        for(int b = -11; b < 11; b++){
            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(),0.2,b + 0.9*random_double());

            if((center - point3(4,0.2,0)).length() > 0.9){
                shared_ptr<material> sphere_material;
                if(choose_mat < 0.8){
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center,0.2,sphere_material));
                } else if(choose_mat < 0.95){
                    auto albedo = color::random(0.5,1);
                    auto fuzz = random_double(0,0.5);
                    sphere_material = make_shared<metal>(albedo,fuzz);
                    world.add(make_shared<sphere>(center,0.2,sphere_material));
                } else {
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center,0.2,sphere_material));
                }
            }
        }
    } */

    auto material1 = make_shared<dielectric>(0.7);
    world.add(make_shared<sphere>(point3(0,1,0),1.0,material1));

    auto material2 = make_shared<lambertian>(color(0.4,0.2,0.6));
    world.add(make_shared<sphere>(point3(-5.5,1,0),1.0,material2));

    auto material3 = make_shared<metal>(color(0.7,0.2,0.5),0.0);
    world.add(make_shared<sphere>(point3(5.5,1,0),1.0,material3));

    

    return world;
}



int main(int argc, char* argv[]) {
    
    const auto aspect_ratio = 3.0 / 2.0;
    const int image_width = 400;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 100;
    const int max_depth = 50;
    
    auto world = random_scene();

    point3 lookfrom(13,2,3);
    point3 lookat(0,0,0);
    vec3 vup(0,1,0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.1;

    camera cam(lookfrom,lookat,vup,20,aspect_ratio,aperture,dist_to_focus);

    BMP bmp(image_width,image_height);

    #pragma omp parallel for
    for(int y = image_height-1; y >= 0; --y){
        std::cerr << "\rScanlines remaining: " << y << ' ' << std::flush;
        for(int x = 0; x < image_width; ++x){

            color pixel_color(0,0,0);
            for(int s = 0; s < samples_per_pixel; s++){
                auto u = (x + random_double()) / (image_width -1 );
                auto v = (y + random_double()) / (image_height - 1);
                ray r = cam.get_ray(u,v);
                pixel_color += ray_color(r,world,max_depth);
            }
            
            bmp.write_color(x,y,pixel_color,samples_per_pixel);
        }
    }
   

    bmp.write("main.bmp");

    std::cerr << "\nEnd of program\n";

    return 0;
}