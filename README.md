# PixelForge2D — ASCII Graphics Editor

PixelForge2D is a lightweight, terminal-based 2D vector graphics editor implemented in C. It allows users to dynamically create, modify, delete, and render geometric shapes (Circles, Rectangles, Lines, and Triangles) onto a fixed-size character canvas using ASCII art representations.

The project showcases advanced C programming concepts, utilizing an object-linked-list architecture to achieve runtime shape polymorphism without relying on massive, hard-to-maintain conditional branch switches.

---

## 🚀 Key Features

* **Polymorphic Shape Rendering:** Every geometric shape manages its own rendering logic dynamically via function pointers bound at creation.
* **Dynamic Scene Management:** Shapes are preserved in a singly linked list with runtime generation of unique tracking IDs and auto-reindexing of ordering layouts.
* **Built-in Demo Scene:** Instantly boots into a predefined landscape matrix (featuring a frame, sun, horizon line, mountain, and house) to verify coordinate systems.
* **Advanced Rasterization Algorithms:**
    * **Lines & Outlines:** Powered by a fast integer-only implementation of **Bresenham's Line Algorithm**.
    * **Circles:** Implemented using the efficient **Midpoint Circle Algorithm** for perfect geometry across a non-square character grid.
    * **Triangles & Fills:** Built on a custom **Scan-Line/Ray-Casting hybrid calculation** to dynamically sweep horizontally across variable slope intercepts.

---

## 🛠️ Software Architecture

Instead of utilizing a monolithic conditional loop to identify and render elements, PixelForge2D assigns drawing operations explicitly to structural instances: