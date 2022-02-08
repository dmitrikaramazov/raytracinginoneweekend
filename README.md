# raytracinginoneweekend
# Simple implementation of raytraced computer graphics

This project was made following along Peter Shirley's Ray Tracing in One Weekend in August of 2021. It differs in a few ways. Importantly, this produces .bmp images rather than the simpler .ppm filetype. I've also tried to customize the materials and add in additional shapes. Additionally, I attempted some optimization and parallel processing using OpenMP.


![example output 1](/saved_images/final_hires_supersampled.bmp)

There are a number of improvements that could be made. Currently, rendering takes several minutes or longer as this is naive ray tracing on the CPU. I'd like to explore methods for both real time raytracing. This program also only can render spheres. Implementing triangles and then meshes should be a relatively easy add on.

### Sources

Peter Shirley's Ray Tracing in One Weekend https://raytracing.github.io/books/RayTracingInOneWeekend.html

BMP header info sources a lot of FileHeader code from https://solarianprogrammer.com/2018/11/19/cpp-reading-writing-bmp-images/

