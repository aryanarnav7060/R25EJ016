# PixelForge2D — ASCII Graphics Editor

PixelForge2D is a terminal-based 2D graphics editor written in C. It allows users to dynamically add, modify, delete, list, and render geometric shapes (Circles, Rectangles, Lines, and Triangles) onto a fixed character grid canvas.

The project uses a singly linked list data structure combined with function pointers to handle shape-specific drawing logic directly inside each node.

---

## 📐 Project Architecture & Structure

The system avoids condition checks inside the rendering loop by assigning a specialized drawing function pointer directly to each individual shape node upon creation.

### 1. Data Structures

#### Shape Type Tag
```c
typedef enum {
    SHAPE_CIRCLE    = 1,
    SHAPE_RECTANGLE = 2,
    SHAPE_LINE      = 3,
    SHAPE_TRIANGLE  = 4
} ShapeType;
```

#### Shape Parameters
```c
typedef struct {
    int  x1, y1;   /* Origin / first point     */
    int  x2, y2;   /* Second point (line/rect) */
    int  x3, y3;   /* Third point  (triangle)  */
    int  radius;   /* Circle radius             */
    char fill;     /* '*' filled, '_' outline   */
} ShapeParams;
```

#### Shape Node
```c
typedef struct ShapeNode {
    int       id;            /* Unique index shown to the user */
    int       list_index;    /* 1-based position in insertion order */
    ShapeType type;
    ShapeParams params;
    char      label[32];     /* Fixed string length of 32 characters */

    /* Function pointer to shape renderer */
    void (*draw)(struct ShapeNode *self, char canvas[30][60]);

    struct ShapeNode *next;
} ShapeNode;
```

### 2. Global Configurations & State Variables
* **Canvas Matrix Size:** 30 Rows $\times$ 60 Columns (`g_canvas[30][60]`)
* **Standard Render Icons:** Empty Space (`' '`), Fill Character (`'*'`), Border/Outline Character (`'_'`)
* **Maximum Node Capacity:** Up to 64 shapes concurrently (`MAX_SHAPES`)
* **Tracking References:**
  * `g_head`: Head pointer of the active shape linked list.
  * `g_next_id`: Auto-incrementing identifier counter (starts at 1).
  * `g_shape_count`: Tracks total active shapes loaded in memory.

---

## ⚡ Algorithms Implemented

### 1. Bresenham's Line Drawing Algorithm (`draw_line_raw`)
Used to plot straight vectors and outline triangle faces. It computes absolute axis step differentials (`dr`, `dc`), tracks direction stepping intervals (`sr`, `sc`), and evaluates an integer error tracker (`err = dr - dc`) to step and alter individual pixel grid spaces.

### 2. Midpoint Circle Optimization (`draw_circle_outline`)
Plots hollow circular parameters using an integer decision tracker variable (`1 - radius`). It tracks coordinates across a single $45^\circ$ boundary quadrant ($y \le x$) and reflects it onto all 8 symmetrical positions of the origin point at the same time using mirroring transformations.

### 3. Scan-Line Triangle Filling (`draw_triangle`)
Iterates row-by-row between the highest and lowest row coordinates of the triangle (`r_min` to `r_max`). It calculates linear path intersection coordinate offsets using pure integer steps:
```c
xs[cnt++] = ec0 + (ec1-ec0)*(r-er0)/(er1-er0);
```
It finds the boundaries of these column cross-sections (`cmin`, `cmax`) and populates the intermediate spaces with the fill character.

---

## 🔄 Memory & CRUD Operations Workflow

### Add Shape Node (`shape_add`)
* Allocates memory via `malloc()` using the size of a `ShapeNode`.
* Stamps the node with a unique tracking ID using `g_next_id++`.
* Appends the instance to the end of the linked list.
* Executes `reindex_list()`, performing a linear traversal to reset sequential 1-based `list_index` labels on all nodes.

### Delete Shape Node (`shape_delete`)
* Iterates through the linked list matching the user-inputted ID tracking target.
* Updates the pointer chain to bypass the designated item (`prev->next = cur->next`).
* Frees the heap memory via `free(cur)`.
* Reduces `g_shape_count` and completely re-indexes the remaining nodes down.

### Modify Shape Node (`handle_modify`)
* Requests target identification ID.
* Triggers the corresponding custom input block (`handle_add_...()`), which appends a brand new temporary node containing the updated dimensions to the tail of the linked list.
* Copies the new configuration array properties back into the target entry via `shape_modify()`.
* Automatically wipes the temporary clone node from memory using `shape_delete()`.

---

## 🕹️ Interactive Workspace Control Map

The program executes an interactive interface console that accepts input selectors mapped as follows:

| Code Menu Selector | Executed Operation | Action Description |
| :---: | :--- | :--- |
| **`1`** | **Add Circle** | Gathers column ($X$), row ($Y$), radius, fill preference (`f`/`o`), and optional name label. |
| **`2`** | **Add Rectangle** | Captures Top-Left coordinates $(X_1, Y_1)$, Bottom-Right coordinates $(X_2, Y_2)$, fill preference, and label. |
| **`3`** | **Add Line** | Plots a vector tracking path between an explicit start and endpoint. |
| **`4`** | **Add Triangle** | Gathers custom spatial location assignments for 3 corner anchor positions. |
| **`5`** | **Delete Shape** | Shows all active shapes and takes an ID selector input to clear a specific node. |
| **`6`** | **Modify Shape** | Rewrites attributes on an active node via the temporary trailing node copy trick. |
| **`7`** | **List Shapes** | Displays an indexed column layout tracking Index, ID, Type, Label, and Fill type settings. |
| **`8`** | **Render & Display** | Wipes the viewport frame array, processes individual shapes in link order, and prints out the canvas matrix. |
| **`9`** | **Clear All Shapes** | Loops through every active allocation node to free heap contents and resets variables. |
| **`0`** | **Exit** | Flushes remaining active allocations from heap memory and gracefully exits the prompt. |