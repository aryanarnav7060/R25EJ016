# PixelForge2D — Polymorphic ASCII Vector Graphics Engine & Editor

PixelForge2D is a high-performance, lightweight, terminal-based 2D vector graphics editor engineered in C. The system enables users to dynamically instantiate, modify, rasterize, and delete geometric shape primitives (Circles, Rectangles, Lines, and Triangles) onto a fixed-size character coordinate canvas. 

Rather than relying on classic procedural control flows, PixelForge2D implements an advanced **object-linked-list architecture** that achieves **runtime shape polymorphism** via dedicated function pointer tables.

---

## 📐 Project Design & Architectural Philosophy

The core engineering objective of PixelForge2D is to eliminate monolithic conditional branching (i.e., massive `switch` or `if-else` blocks inside rendering loops) when drawing disparate shape types. This directly enforces the **Open-Closed Principle** from object-oriented design: the rendering system is open for extension (adding new shapes) but closed for modification.

```
                  [ Global Head Reference Anchor ]
                                │
                                ▼
               +───────────────────────────────────+
               │ ShapeNode Instance: SHAPE_CIRCLE  │
               │ - Params: x1, y1, radius          │
               │ - *draw ───► points to:           │──────► draw_circle()
               +───────────────────────────────────+
                                │
                                ▼
               +───────────────────────────────────+
               │ ShapeNode Instance: SHAPE_LINE    │
               │ - Params: x1, y1, x2, y2          │
               │ - *draw ───► points to:           │──────► draw_line_shape()
               +───────────────────────────────────+
                                │
                                ▼
                              NULL
```

### 1. Data Structure Layout
Every geometric entity is encapsulated inside a self-contained `ShapeNode` structure. This serves as both a data container and an interface behavior carrier:

```c
typedef struct ShapeNode {
    int       id;            /* Persistent primary key assigned sequentially */
    int       list_index;    /* Dynamic 1-based chronological placement marker */
    ShapeType type;          /* Enumerated geometric class identifier */
    ShapeParams params;      /* Raw coordinate and boundary attributes union */
    char      label[32];     /* Alphanumeric user tag for scene tracking */

    /* The V-Table Interface Link: Polymorphic Function Pointer */
    void (*draw)(struct ShapeNode *self, char canvas[30][60]);

    struct ShapeNode *next;  /* Singly-linked forward traversal pointer */
} ShapeNode;
```

### 2. Dynamic V-Table Binding Workflow
When a shape is requested via `shape_add()`, a generic node is generated via `make_node()`. During execution, the system dynamically binds the specific low-level rendering function to that instance's function pointer field:

```c
switch (type) {
    case SHAPE_CIRCLE:    node->draw = draw_circle;     break;
    case SHAPE_RECTANGLE: node->draw = draw_rectangle;  break;
    case SHAPE_LINE:      node->draw = draw_line_shape; break;
    case SHAPE_TRIANGLE:  node->draw = draw_triangle;   break;
    default:              node->draw = NULL;
}
```

When `scene_render()` runs, it loops through the active nodes and invokes the pointer directly:
```c
while (cur) {
    if (cur->draw) cur->draw(cur, g_canvas);
    cur = cur->next;
}
```

---

## ⚡ Mathematical & Algorithmic Analysis

PixelForge2D uses pure integer-based computer graphics algorithms. Because terminal grids lack floating-point sub-pixel rendering and processing power can vary, using pure integers avoids slow floating-point math and costly square root or trigonometric calls.

### 1. Bresenham's Midpoint Line Algorithm
Used to draw lines and triangle boundaries (`draw_line_raw`). It relies entirely on addition and subtraction to trace the closest pixel match along a true vector path.

* **Mathematical Foundations:** Given an initial error margin defined by coordinate differences:
  $$\text{err} = \Delta r - \Delta c$$
* For every step along the primary axis, the system evaluates the decision parameter ($2 \times \text{err}$). If it exceeds the threshold, it steps along the secondary axis and subtracts the pixel delta constraint, stabilizing pixel drift without any division operations.

### 2. Midpoint Circle Optimization
Used to draw circular paths (`draw_circle_outline`). It uses an integer decision variable to track the mathematical path of the curve:
$$F(x, y) = x^2 + y^2 - R^2$$

* **Eight-Way Symmetry Reflection:** To maximize performance, the algorithm only calculates coordinates for a single $45^\circ$ octant arc ($y \le x$). Once a pixel is evaluated, it mirrors it across all eight Cartesian octants simultaneously:
  ```c
  canvas_set(cr + y, cc + x, ch);     canvas_set(cr - y, cc + x, ch);
  canvas_set(cr + y, cc - x, ch);     canvas_set(cr - y, cc - x, ch);
  ```

### 3. Scan-Line Triangle Boundary Interpolation
Used for solid shapes (`draw_triangle`). The engine calculates the maximum vertical span (`r_min` to `r_max`) and processes row intersections line-by-line.

```
       Vertex 1 (r0, c0)
             /\
            /  \  ◄─────── Scanning Row Line (r)
  -------- /────\ -------- Intersection Point: c = c0 + (c1-c0)*(r-r0)/(r1-r0)
          /______\
Vertex 2          Vertex 3
(r1, c1)          (r2, c2)
```

* **Linear Scan Translation:** For each row coordinate $r$, the horizontal axis intercept point $c$ for an edge connecting points $(r_0, c_0)$ and $(r_1, c_1)$ is derived via linear interpolation:
  $$c = c_0 + \frac{(c_1 - c_0) \times (r - r_0)}{r_1 - r_0}$$
* The system tracks column intersection arrays (`xs`), extracts the boundary limits (`cmin`, `cmax`), and floods the intervening span.

---

## 🔄 State Machine & CRUD Memory Flow

The workspace coordinates canvas data like an in-memory transactional database. Below is the lifecycle tracking for its data updates:

### Creation Flow (`shape_add`)
1. Memory is allocated on the heap for a new node instance.
2. The current `g_next_id` counter value is stamped as a permanent ID key.
3. The instance is appended to the tail end of the linked list.
4. `reindex_list()` executes a quick linear pass to refresh structural index alignments.

### Modification Workflow (`handle_modify`)
To maximize code reuse and avoid duplicate input logic, modifications use a smart **clone-and-swap pattern**:
1. The user picks an active target shape ID.
2. The engine calls the relevant `handle_add_...()` sequence, prompting the user for fresh attributes. This temporarily builds a new clone node at the tail of the list.
3. `shape_modify()` extracts the parameters from this temporary node and updates the target node.
4. The temporary clone node is safely freed via `shape_delete()` once the update is complete.

### Deletion Flow (`shape_delete`)
1. The system loops through nodes until it finds a matching ID key.
2. Pointer links bypass the target node: `prev->next = cur->next`.
3. `free(cur)` safely releases the node's heap allocation.
4. `reindex_list()` automatically shifts index order numbers down to maintain an unbroken UI table list.

---

## 🛠️ System Specifications & Build Matrix

### Core Hardcoded Bounds
* **Canvas Dimensions:** 30 Rows $\times$ 60 Columns Matrix Grid.
* **Maximum Object Stack Size:** 64 concurrent nodes.
* **String Allocation Limits:** Maximum 32 characters for custom shape labels.

### Compilation Instructions
This code relies exclusively on the standard library definitions, ensuring zero external third-party dependencies.

```bash
# Production-grade optimization compilation command line
gcc -Wall -Wextra -O3 graphics_editor.c -o pixelforge2d -lm

# Debug configuration profile compilation command line
gcc -g -fsanitize=address graphics_editor.c -o pixelforge2d_debug -lm
```
*Note: The math link flag (`-lm`) is required by the linker to resolve integer math utilities like `abs()` or geometric divisions.*

---

## 🕹️ Interactive Interface Control Layout

When you run the binary, an interactive prompt handles user input:

| Code | Operations | Execution Context |
| :---: | :--- | :--- |
| **`1`** | **Add Circle** | Asks for Center $(X, Y)$, Radius, and Outline/Fill preference. |
| **`2`** | **Add Rectangle** | Asks for Top-Left $(X_1, Y_1)$ and Bottom-Right $(X_2, Y_2)$ corners. |
| **`3`** | **Add Line** | Plots a vector line using Bresenham's algorithm between two points. |
| **`4`** | **Add Triangle** | Asks for three separate vertex point pairs. |
| **`5`** | **Delete Shape** | Removes a node by its ID and shifts list index order down. |
| **`6`** | **Modify Shape** | Uses the swap pattern to overwrite an existing shape's parameters. |
| **`7`** | **List Shapes** | Prints a table tracking list index positions, IDs, and labels. |
| **`8`** | **Render Canvas** | Clears the buffer, re-draws all active nodes, and displays the ASCII matrix. |
| **`9`** | **Clear All** | Frees all node allocations on the heap and resets the workspace. |
| **`0`** | **Exit** | Cleans up all remaining heap memory allocations and terminates execution. |

---

## 🛡️ Robust Memory & Input Handling

* **Buffer Overflow Prevention:** The system avoids insecure inputs like `gets()`. It uses `fgets()` for label strings, ensuring inputs never overflow the 32-character buffer allocation limits.
* **Input Stream Cleansing:** Input buffers are consistently flushed via `while (getchar() != '\n');` lines. This wipes residual newline junk characters, preventing menu skipping or infinite looping bugs on invalid inputs.
* **Leak Mitigation Assurance:** Every allocation path is matched with a corresponding cleanup routine. Calling options `9` or `0` safely runs a full traversal link loop to free every node allocation, ensuring zero memory leaks upon exit.