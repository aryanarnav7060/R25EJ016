/*
 * PixelForge2D - A 2D ASCII Graphics Editor
 * Uses '*' for filled pixels and '_' for borders/lines
 *
 * Architecture: Object-linked-list with shape polymorphism via function pointers
 * Each shape knows how to draw itself — no giant switch statements needed.
 *
 * Author: Student
 * Date:   2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ─────────────── Canvas Configuration ─────────────── */
#define CANVAS_ROWS    30
#define CANVAS_COLS    60
#define EMPTY_CHAR     ' '
#define FILL_CHAR      '*'
#define BORDER_CHAR    '_'
#define MAX_SHAPES     64
#define SHAPE_NAME_LEN 32

/* ─────────────── Shape Type Tags ─────────────── */
typedef enum {
    SHAPE_CIRCLE    = 1,
    SHAPE_RECTANGLE = 2,
    SHAPE_LINE      = 3,
    SHAPE_TRIANGLE  = 4
} ShapeType;

/* ─────────────── Shape Parameter Struct ─────────────── */
typedef struct {
    int  x1, y1;   /* Origin / first point     */
    int  x2, y2;   /* Second point (line/rect) */
    int  x3, y3;   /* Third point  (triangle)  */
    int  radius;   /* Circle radius             */
    char fill;     /* '*' filled, '_' outline   */
} ShapeParams;

/* ─────────────── Shape Node (linked list) ─────────────── */
typedef struct ShapeNode {
    int       id;            /* Unique index shown to the user */
    int       list_index;   /* 1-based position in insertion order */
    ShapeType type;
    ShapeParams params;
    char      label[SHAPE_NAME_LEN];

    /* Polymorphic draw — each node carries its own renderer */
    void (*draw)(struct ShapeNode *self, char canvas[CANVAS_ROWS][CANVAS_COLS]);

    struct ShapeNode *next;
} ShapeNode;

/* ─────────────── Global State ─────────────── */
static char       g_canvas[CANVAS_ROWS][CANVAS_COLS];
static ShapeNode *g_head        = NULL;
static int        g_next_id     = 1;
static int        g_shape_count = 0;

/* ══════════════════════════════════════════════════════
 *  INDEX HELPERS
 * ══════════════════════════════════════════════════════ */

/* Rebuild the 1-based list_index for every node after any change */
static void reindex_list(void) {
    int idx = 1;
    ShapeNode *cur = g_head;
    while (cur) { cur->list_index = idx++; cur = cur->next; }
}

static const char *type_name(ShapeType t) {
    switch (t) {
        case SHAPE_CIRCLE:    return "Circle";
        case SHAPE_RECTANGLE: return "Rectangle";
        case SHAPE_LINE:      return "Line";
        case SHAPE_TRIANGLE:  return "Triangle";
        default:              return "Unknown";
    }
}

/* ══════════════════════════════════════════════════════
 *  CANVAS UTILITIES
 * ══════════════════════════════════════════════════════ */

void canvas_clear(void) {
    for (int r = 0; r < CANVAS_ROWS; r++)
        for (int c = 0; c < CANVAS_COLS; c++)
            g_canvas[r][c] = EMPTY_CHAR;
}

static inline void canvas_set(int row, int col, char ch) {
    if (row >= 0 && row < CANVAS_ROWS && col >= 0 && col < CANVAS_COLS)
        g_canvas[row][col] = ch;
}

void canvas_display(void) {
    /* Header shows total shape count */
    printf("\n  Canvas  [%d shape(s) loaded]\n", g_shape_count);
    printf("  Columns: 0                   30                  59\n");
    printf("           |                    |                   |\n");

    /* Top border with column ruler every 10 chars */
    printf("  Row 00 +");
    for (int c = 0; c < CANVAS_COLS; c++) printf("-");
    printf("+\n");

    for (int r = 0; r < CANVAS_ROWS; r++) {
        printf("  Row %02d |", r);
        for (int c = 0; c < CANVAS_COLS; c++)
            putchar(g_canvas[r][c]);
        printf("|\n");
    }

    printf("         +");
    for (int c = 0; c < CANVAS_COLS; c++) printf("-");
    printf("+\n");
}

/* ══════════════════════════════════════════════════════
 *  DRAWING PRIMITIVES
 * ══════════════════════════════════════════════════════ */

/* Bresenham line */
static void draw_line_raw(int r0, int c0, int r1, int c1, char ch) {
    int dr = abs(r1 - r0), dc = abs(c1 - c0);
    int sr = (r0 < r1) ? 1 : -1;
    int sc = (c0 < c1) ? 1 : -1;
    int err = dr - dc;

    while (1) {
        canvas_set(r0, c0, ch);
        if (r0 == r1 && c0 == c1) break;
        int e2 = 2 * err;
        if (e2 > -dc) { err -= dc; r0 += sr; }
        if (e2 <  dr) { err += dr; c0 += sc; }
    }
}

/* Midpoint circle outline */
static void draw_circle_outline(int cr, int cc, int radius, char ch) {
    int x = radius, y = 0;
    int decision = 1 - radius;
    while (y <= x) {
        canvas_set(cr + y, cc + x, ch); canvas_set(cr - y, cc + x, ch);
        canvas_set(cr + y, cc - x, ch); canvas_set(cr - y, cc - x, ch);
        canvas_set(cr + x, cc + y, ch); canvas_set(cr - x, cc + y, ch);
        canvas_set(cr + x, cc - y, ch); canvas_set(cr - x, cc - y, ch);
        y++;
        if (decision <= 0) { decision += 2 * y + 1; }
        else               { x--; decision += 2 * (y - x) + 1; }
    }
}

/* Filled circle — scan-line */
static void draw_circle_filled(int cr, int cc, int radius, char ch) {
    for (int r = cr - radius; r <= cr + radius; r++)
        for (int c = cc - radius; c <= cc + radius; c++) {
            int dr = r - cr, dc = c - cc;
            if (dr * dr + dc * dc <= radius * radius)
                canvas_set(r, c, ch);
        }
}

/* ══════════════════════════════════════════════════════
 *  SHAPE DRAW FUNCTIONS  (attached to each node)
 * ══════════════════════════════════════════════════════ */

static void draw_circle(ShapeNode *self, char canvas[CANVAS_ROWS][CANVAS_COLS]) {
    (void)canvas;
    ShapeParams *p = &self->params;
    if (p->fill == FILL_CHAR) draw_circle_filled (p->y1, p->x1, p->radius, p->fill);
    else                      draw_circle_outline(p->y1, p->x1, p->radius, BORDER_CHAR);
}

static void draw_rectangle(ShapeNode *self, char canvas[CANVAS_ROWS][CANVAS_COLS]) {
    (void)canvas;
    ShapeParams *p = &self->params;
    int r1 = p->y1, c1 = p->x1, r2 = p->y2, c2 = p->x2;
    if (r1 > r2) { int t = r1; r1 = r2; r2 = t; }
    if (c1 > c2) { int t = c1; c1 = c2; c2 = t; }
    if (p->fill == FILL_CHAR) {
        for (int r = r1; r <= r2; r++)
            for (int c = c1; c <= c2; c++)
                canvas_set(r, c, FILL_CHAR);
    } else {
        for (int c = c1; c <= c2; c++) { canvas_set(r1, c, BORDER_CHAR); canvas_set(r2, c, BORDER_CHAR); }
        for (int r = r1; r <= r2; r++) { canvas_set(r, c1, BORDER_CHAR); canvas_set(r, c2, BORDER_CHAR); }
    }
}

static void draw_line_shape(ShapeNode *self, char canvas[CANVAS_ROWS][CANVAS_COLS]) {
    (void)canvas;
    ShapeParams *p = &self->params;
    draw_line_raw(p->y1, p->x1, p->y2, p->x2,
                  (p->fill == FILL_CHAR) ? FILL_CHAR : BORDER_CHAR);
}

static void draw_triangle(ShapeNode *self, char canvas[CANVAS_ROWS][CANVAS_COLS]) {
    (void)canvas;
    ShapeParams *p = &self->params;
    char ch = (p->fill == FILL_CHAR) ? FILL_CHAR : BORDER_CHAR;
    if (p->fill == FILL_CHAR) {
        int r_min = p->y1, r_max = p->y1;
        if (p->y2 < r_min) r_min = p->y2; if (p->y3 < r_min) r_min = p->y3;
        if (p->y2 > r_max) r_max = p->y2; if (p->y3 > r_max) r_max = p->y3;
        for (int r = r_min; r <= r_max; r++) {
            int xs[6], cnt = 0;
            int pts_r[3] = { p->y1, p->y2, p->y3 };
            int pts_c[3] = { p->x1, p->x2, p->x3 };
            for (int e = 0; e < 3; e++) {
                int er0 = pts_r[e], ec0 = pts_c[e];
                int er1 = pts_r[(e+1)%3], ec1 = pts_c[(e+1)%3];
                if ((r >= er0 && r <= er1) || (r >= er1 && r <= er0)) {
                    if (er0 == er1) { xs[cnt++] = ec0; xs[cnt++] = ec1; }
                    else xs[cnt++] = ec0 + (ec1-ec0)*(r-er0)/(er1-er0);
                }
            }
            if (cnt >= 2) {
                int cmin = xs[0], cmax = xs[0];
                for (int i = 1; i < cnt; i++) {
                    if (xs[i] < cmin) cmin = xs[i];
                    if (xs[i] > cmax) cmax = xs[i];
                }
                for (int c = cmin; c <= cmax; c++) canvas_set(r, c, ch);
            }
        }
    } else {
        draw_line_raw(p->y1, p->x1, p->y2, p->x2, ch);
        draw_line_raw(p->y2, p->x2, p->y3, p->x3, ch);
        draw_line_raw(p->y3, p->x3, p->y1, p->x1, ch);
    }
}

/* ══════════════════════════════════════════════════════
 *  LINKED-LIST MANAGEMENT
 * ══════════════════════════════════════════════════════ */

static ShapeNode *make_node(ShapeType type, ShapeParams params, const char *label) {
    ShapeNode *node = (ShapeNode *)malloc(sizeof(ShapeNode));
    if (!node) { perror("malloc"); exit(1); }
    node->id         = g_next_id++;
    node->list_index = 0;   /* set by reindex_list() */
    node->type       = type;
    node->params     = params;
    strncpy(node->label, label, SHAPE_NAME_LEN - 1);
    node->label[SHAPE_NAME_LEN - 1] = '\0';
    node->next = NULL;
    switch (type) {
        case SHAPE_CIRCLE:    node->draw = draw_circle;     break;
        case SHAPE_RECTANGLE: node->draw = draw_rectangle;  break;
        case SHAPE_LINE:      node->draw = draw_line_shape; break;
        case SHAPE_TRIANGLE:  node->draw = draw_triangle;   break;
        default:              node->draw = NULL;
    }
    return node;
}

int shape_add(ShapeType type, ShapeParams params, const char *label) {
    if (g_shape_count >= MAX_SHAPES) {
        printf("[!] Canvas is full (%d shapes max).\n", MAX_SHAPES);
        return -1;
    }
    ShapeNode *node = make_node(type, params, label);
    if (!g_head) {
        g_head = node;
    } else {
        ShapeNode *cur = g_head;
        while (cur->next) cur = cur->next;
        cur->next = node;
    }
    g_shape_count++;
    reindex_list();
    printf("[+] Added  Index #%d | ID %d | %s | label='%s'\n",
           node->list_index, node->id, type_name(type), label);
    return node->id;
}

int shape_delete(int id) {
    ShapeNode *prev = NULL, *cur = g_head;
    while (cur) {
        if (cur->id == id) {
            if (prev) prev->next = cur->next;
            else      g_head     = cur->next;
            printf("[-] Deleted  Index #%d | ID %d | %s | label='%s'\n",
                   cur->list_index, cur->id, type_name(cur->type), cur->label);
            free(cur);
            g_shape_count--;
            reindex_list();   /* indexes shift after deletion */
            return 1;
        }
        prev = cur; cur = cur->next;
    }
    printf("[!] Shape ID %d not found.\n", id);
    return 0;
}

int shape_modify(int id, ShapeParams new_params, const char *new_label) {
    ShapeNode *cur = g_head;
    while (cur) {
        if (cur->id == id) {
            cur->params = new_params;
            if (new_label && new_label[0])
                strncpy(cur->label, new_label, SHAPE_NAME_LEN - 1);
            printf("[~] Modified  Index #%d | ID %d | %s | label='%s'\n",
                   cur->list_index, cur->id, type_name(cur->type), cur->label);
            return 1;
        }
        cur = cur->next;
    }
    printf("[!] Shape ID %d not found.\n", id);
    return 0;
}

void scene_render(void) {
    canvas_clear();
    ShapeNode *cur = g_head;
    while (cur) {
        if (cur->draw) cur->draw(cur, g_canvas);
        cur = cur->next;
    }
}

/* Full indexed shape listing */
void shapes_list(void) {
    if (!g_head) { printf("  (no shapes in scene)\n"); return; }
    printf("\n");
    printf("  %-6s  %-4s  %-12s  %-20s  %s\n",
           "Index", "ID", "Type", "Label", "Fill");
    printf("  ------  ----  ------------  --------------------  ----\n");
    ShapeNode *cur = g_head;
    while (cur) {
        printf("  #%-5d  %-4d  %-12s  %-20s  %c\n",
               cur->list_index,
               cur->id,
               type_name(cur->type),
               cur->label,
               cur->params.fill);
        cur = cur->next;
    }
    printf("\n  Total: %d shape(s)\n", g_shape_count);
}

void shapes_free_all(void) {
    ShapeNode *cur = g_head;
    while (cur) { ShapeNode *tmp = cur->next; free(cur); cur = tmp; }
    g_head = NULL; g_shape_count = 0;
}

/* ══════════════════════════════════════════════════════
 *  INTERACTIVE MENU
 * ══════════════════════════════════════════════════════ */

static void print_banner(void) {
    printf("\n");
    printf("  +--------------------------------------------------+\n");
    printf("  |       PixelForge2D  -  ASCII Graphics Editor     |\n");
    printf("  |       Uses '*' for fill  and  '_' for outlines   |\n");
    printf("  |       Each shape gets a unique ID + list Index   |\n");
    printf("  +--------------------------------------------------+\n\n");
}

static void print_menu(void) {
    printf("\n  ------------------------------------------\n");
    printf("  MENU                    [%d shape(s) active]\n", g_shape_count);
    printf("  ------------------------------------------\n");
    printf("  1. Add Circle\n");
    printf("  2. Add Rectangle\n");
    printf("  3. Add Line\n");
    printf("  4. Add Triangle\n");
    printf("  5. Delete Shape   (enter ID)\n");
    printf("  6. Modify Shape   (enter ID)\n");
    printf("  7. List Shapes    (shows Index + ID)\n");
    printf("  8. Render & Display Canvas\n");
    printf("  9. Clear All Shapes\n");
    printf("  0. Exit\n");
    printf("  ------------------------------------------\n");
    printf("  Choice: ");
}

static char ask_fill(void) {
    char buf[8];
    printf("  Fill? (f=filled / o=outline): ");
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return BORDER_CHAR;
    return (buf[0] == 'f' || buf[0] == 'F') ? FILL_CHAR : BORDER_CHAR;
}

static void handle_add_circle(void) {
    ShapeParams p = {0};
    char label[SHAPE_NAME_LEN];
    printf("  Centre col (x, 0-%d): ", CANVAS_COLS-1); scanf("%d", &p.x1);
    printf("  Centre row (y, 0-%d): ", CANVAS_ROWS-1); scanf("%d", &p.y1);
    printf("  Radius              : "); scanf("%d", &p.radius);
    while (getchar() != '\n');
    p.fill = ask_fill();
    printf("  Label               : "); fgets(label, sizeof(label), stdin);
    label[strcspn(label, "\n")] = '\0';
    shape_add(SHAPE_CIRCLE, p, label[0] ? label : "circle");
}

static void handle_add_rectangle(void) {
    ShapeParams p = {0};
    char label[SHAPE_NAME_LEN];
    printf("  Top-left  col (x1, 0-%d): ", CANVAS_COLS-1); scanf("%d", &p.x1);
    printf("  Top-left  row (y1, 0-%d): ", CANVAS_ROWS-1); scanf("%d", &p.y1);
    printf("  Bot-right col (x2, 0-%d): ", CANVAS_COLS-1); scanf("%d", &p.x2);
    printf("  Bot-right row (y2, 0-%d): ", CANVAS_ROWS-1); scanf("%d", &p.y2);
    while (getchar() != '\n');
    p.fill = ask_fill();
    printf("  Label                    : "); fgets(label, sizeof(label), stdin);
    label[strcspn(label, "\n")] = '\0';
    shape_add(SHAPE_RECTANGLE, p, label[0] ? label : "rect");
}

static void handle_add_line(void) {
    ShapeParams p = {0};
    char label[SHAPE_NAME_LEN];
    printf("  Start col (x1, 0-%d): ", CANVAS_COLS-1); scanf("%d", &p.x1);
    printf("  Start row (y1, 0-%d): ", CANVAS_ROWS-1); scanf("%d", &p.y1);
    printf("  End   col (x2, 0-%d): ", CANVAS_COLS-1); scanf("%d", &p.x2);
    printf("  End   row (y2, 0-%d): ", CANVAS_ROWS-1); scanf("%d", &p.y2);
    while (getchar() != '\n');
    p.fill = ask_fill();
    printf("  Label               : "); fgets(label, sizeof(label), stdin);
    label[strcspn(label, "\n")] = '\0';
    shape_add(SHAPE_LINE, p, label[0] ? label : "line");
}

static void handle_add_triangle(void) {
    ShapeParams p = {0};
    char label[SHAPE_NAME_LEN];
    printf("  Vertex 1 col (x1, 0-%d): ", CANVAS_COLS-1); scanf("%d", &p.x1);
    printf("  Vertex 1 row (y1, 0-%d): ", CANVAS_ROWS-1); scanf("%d", &p.y1);
    printf("  Vertex 2 col (x2, 0-%d): ", CANVAS_COLS-1); scanf("%d", &p.x2);
    printf("  Vertex 2 row (y2, 0-%d): ", CANVAS_ROWS-1); scanf("%d", &p.y2);
    printf("  Vertex 3 col (x3, 0-%d): ", CANVAS_COLS-1); scanf("%d", &p.x3);
    printf("  Vertex 3 row (y3, 0-%d): ", CANVAS_ROWS-1); scanf("%d", &p.y3);
    while (getchar() != '\n');
    p.fill = ask_fill();
    printf("  Label                   : "); fgets(label, sizeof(label), stdin);
    label[strcspn(label, "\n")] = '\0';
    shape_add(SHAPE_TRIANGLE, p, label[0] ? label : "triangle");
}

static void handle_delete(void) {
    shapes_list();
    int id;
    printf("  Enter ID to delete: "); scanf("%d", &id);
    while (getchar() != '\n');
    shape_delete(id);
}

static void handle_modify(void) {
    shapes_list();
    int id;
    printf("  Enter ID to modify: "); scanf("%d", &id);
    while (getchar() != '\n');

    ShapeNode *cur = g_head;
    while (cur && cur->id != id) cur = cur->next;
    if (!cur) { printf("[!] Shape ID %d not found.\n", id); return; }

    printf("  Re-enter parameters for [Index #%d | ID %d | %s]:\n",
           cur->list_index, cur->id, type_name(cur->type));

    switch (cur->type) {
        case SHAPE_CIRCLE:    handle_add_circle();    break;
        case SHAPE_RECTANGLE: handle_add_rectangle(); break;
        case SHAPE_LINE:      handle_add_line();      break;
        case SHAPE_TRIANGLE:  handle_add_triangle();  break;
    }

    /* Swap new node's params into the original id, then remove the duplicate */
    ShapeNode *newest = g_head;
    while (newest->next) newest = newest->next;
    if (newest->id != id) {
        shape_modify(id, newest->params, newest->label);
        shape_delete(newest->id);
    }
}

/* ══════════════════════════════════════════════════════
 *  DEMO SCENE
 * ══════════════════════════════════════════════════════ */
static void demo_scene(void) {
    ShapeParams p;
    p = (ShapeParams){.x1=1,  .y1=1,  .x2=58, .y2=28, .fill=BORDER_CHAR};
    shape_add(SHAPE_RECTANGLE, p, "frame");

    p = (ShapeParams){.x1=50, .y1=5,  .radius=4, .fill=FILL_CHAR};
    shape_add(SHAPE_CIRCLE, p, "sun");

    p = (ShapeParams){.x1=20, .y1=25, .x2=35, .y2=8, .x3=50, .y3=25, .fill=BORDER_CHAR};
    shape_add(SHAPE_TRIANGLE, p, "mountain");

    p = (ShapeParams){.x1=2,  .y1=22, .x2=57, .y2=22, .fill=BORDER_CHAR};
    shape_add(SHAPE_LINE, p, "horizon");

    p = (ShapeParams){.x1=8,  .y1=16, .x2=18, .y2=22, .fill=FILL_CHAR};
    shape_add(SHAPE_RECTANGLE, p, "house-body");
}

/* ══════════════════════════════════════════════════════
 *  MAIN
 * ══════════════════════════════════════════════════════ */
int main(void) {
    print_banner();
    canvas_clear();

    printf("  Loading demo scene...\n");
    demo_scene();
    printf("\n  Scene index after load:\n");
    shapes_list();
    scene_render();
    canvas_display();

    int running = 1;
    while (running) {
        print_menu();
        int choice;
        if (scanf("%d", &choice) != 1) { while (getchar() != '\n'); continue; }
        while (getchar() != '\n');

        switch (choice) {
            case 1: handle_add_circle();    break;
            case 2: handle_add_rectangle(); break;
            case 3: handle_add_line();      break;
            case 4: handle_add_triangle();  break;
            case 5: handle_delete();        break;
            case 6: handle_modify();        break;
            case 7:
                printf("\n  Shape index (%d total):\n", g_shape_count);
                shapes_list();
                break;
            case 8:
                scene_render();
                canvas_display();
                break;
            case 9:
                shapes_free_all();
                canvas_clear();
                printf("[*] All shapes cleared. Index is now empty.\n");
                break;
            case 0:
                running = 0;
                break;
            default:
                printf("[!] Unknown option.\n");
        }
    }

    shapes_free_all();
    printf("\n  Goodbye from PixelForge2D!\n");
    return 0;
}
