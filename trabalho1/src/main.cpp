#include <raylib.h>
#include <memory>
#include <vector>
#include <string>
#include "raymath.h"
#include <cmath>
#include <fstream>
#include <chrono>

static Color RandomColor()
{
	return {(unsigned char)GetRandomValue(0, 200),
			(unsigned char)GetRandomValue(0, 200),
			(unsigned char)GetRandomValue(0, 200),
			255};
}

bool CheckCollisionPointLine(Vector2 point, Vector2 a, Vector2 b, float thickness)
{
	float d = fabsf((b.y - a.y) * point.x - (b.x - a.x) * point.y + b.x * a.y - b.y * a.x) /
			  Vector2Distance(a, b);
	return d <= thickness;
}

struct Shape
{
	virtual void Draw() = 0;
	// HandleInput: deve ser chamado todo frame e retorna true quando a forma ficar pronta
	virtual bool HandleInput() { return true; }
	virtual bool Contains(Vector2 p) = 0;
	virtual void Move(Vector2 delta) = 0;
	virtual ~Shape() = default;
};

// ponto (1 clique)
struct PointShape : public Shape
{
	Vector2 pos{};
	bool waiting = true;
	bool ready = false;
	Color color;

	PointShape() : color(RandomColor()) {}

	bool HandleInput() override
	{
		if (waiting && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
		{
			pos = GetMousePosition();
			waiting = false;
			ready = true;
		}
		return ready;
	}

	void Draw() override
	{
		if (!ready)
		{
			// preview: circulo pequeno em volta do mouse
			DrawCircleV(GetMousePosition(), 4.0f, GRAY);
		}
		else
		{
			DrawCircleV(pos, 4.0f, color);
		}
	}

	bool Contains(Vector2 p) override
	{
		return CheckCollisionPointCircle(p, pos, 6.0f);
	}

	void Move(Vector2 delta) override
	{
		pos = Vector2Add(pos, delta);
	}
};

// linha (2 cliques)
struct LineShape : public Shape
{
	Vector2 a{}, b{};
	bool waitingA = true;
	bool ready = false;
	Color color;

	LineShape() : color(RandomColor()) {}

	bool HandleInput() override
	{
		if (waitingA)
		{
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				a = GetMousePosition();
				waitingA = false;
			}
		}
		else if (!ready)
		{
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				b = GetMousePosition();
				ready = true;
			}
		}
		return ready;
	}

	void Draw() override
	{
		if (waitingA)
		{
			// preview: ponto do mouse
			DrawCircleV(GetMousePosition(), 4.f, GRAY);
		}
		else if (!ready)
		{
			// preview: linha do ponto inicial até o mouse
			DrawLineV(a, GetMousePosition(), GRAY);
			DrawCircleV(a, 4.f, color);
		}
		else
		{
			DrawLineV(a, b, color);
			DrawCircleV(a, 4.f, color);
			DrawCircleV(b, 4.f, color);
		}
	}

	bool Contains(Vector2 p) override
	{
		return CheckCollisionPointLine(p, a, b, 5.0f);
	}

	void Move(Vector2 delta) override
	{
		a = Vector2Add(a, delta);
		b = Vector2Add(b, delta);
	}
};

// círculo (2 cliques: centro + ponto na circunferência)
struct CircleShape : public Shape
{
	Vector2 center{};
	float radius = 0.f;
	bool waitingCenter = true;
	bool ready = false;
	Color color;

	CircleShape() : color(RandomColor()) {}

	bool HandleInput() override
	{
		if (waitingCenter)
		{
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				center = GetMousePosition();
				waitingCenter = false;
			}
		}
		else if (!ready)
		{
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				Vector2 p = GetMousePosition();
				radius = Vector2Distance(center, p);
				ready = true;
			}
		}
		return ready;
	}

	void Draw() override
	{
		if (waitingCenter)
		{
			DrawCircleV(GetMousePosition(), 4.f, GRAY);
		}
		else if (!ready)
		{
			float r = Vector2Distance(center, GetMousePosition());
			DrawCircleLines(center.x, center.y, r, GRAY);
			DrawCircleV(center, 3.f, color);
		}
		else
		{
			DrawCircleV(center, radius, color);
		}
	}

	bool Contains(Vector2 p) override
	{
		return CheckCollisionPointCircle(p, center, radius);
	}

	void Move(Vector2 delta) override
	{
		center = Vector2Add(center, delta);
	}
};

// retângulo (2 cliques: canto1, canto2)
struct RectShape : public Shape
{
	Vector2 p1{}, p2{};
	bool waitingP1 = true;
	bool ready = false;
	Color color;

	RectShape() : color(RandomColor()) {}

	bool HandleInput() override
	{
		if (waitingP1)
		{
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				p1 = GetMousePosition();
				waitingP1 = false;
			}
		}
		else if (!ready)
		{
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				p2 = GetMousePosition();
				ready = true;
			}
		}
		return ready;
	}

	void Draw() override
	{
		if (waitingP1)
		{
			DrawCircleV(GetMousePosition(), 3.f, GRAY);
		}
		else if (!ready)
		{
			Vector2 m = GetMousePosition();
			float x = p1.x;
			float y = p1.y;
			float w = m.x - p1.x;
			float h = m.y - p1.y;
			DrawRectangleLinesEx({x, y, w, h}, 2, GRAY);
		}
		else
		{
			float x = p1.x < p2.x ? p1.x : p2.x;
			float y = p1.y < p2.y ? p1.y : p2.y;
			float w = fabsf(p2.x - p1.x);
			float h = fabsf(p2.y - p1.y);
			DrawRectangleRec({x, y, w, h}, color);
		}
	}

	bool Contains(Vector2 p) override
	{
		Rectangle r{
			fminf(p1.x, p2.x), fminf(p1.y, p2.y),
			fabsf(p2.x - p1.x), fabsf(p2.y - p1.y)};
		return CheckCollisionPointRec(p, r);
	}

	void Move(Vector2 delta) override
	{
		p1 = Vector2Add(p1, delta);
		p2 = Vector2Add(p2, delta);
	}
};

// triângulo (3 cliques)
struct TriangleShape : public Shape
{
	Vector2 pts[3]{};
	int count = 0;
	bool ready = false;
	Color color;

	TriangleShape() : color(RandomColor()) {}

	bool HandleInput() override
	{
		if (count < 3 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
		{
			pts[count++] = GetMousePosition();
			if (count == 3)
				ready = true;
		}
		return ready;
	}

	void Draw() override
	{
		if (count == 0)
		{
			DrawCircleV(GetMousePosition(), 3.f, GRAY);
		}
		else if (count == 1)
		{
			DrawLineV(pts[0], GetMousePosition(), GRAY);
			DrawCircleV(pts[0], 3.f, color);
		}
		else if (count == 2)
		{
			DrawTriangleLines(pts[0], pts[1], GetMousePosition(), GRAY);
			DrawCircleV(pts[0], 3.f, color);
			DrawCircleV(pts[1], 3.f, color);
		}
		else
		{
			DrawTriangle(pts[0], pts[1], pts[2], color);
		}
	}

	bool Contains(Vector2 p) override
	{
		return CheckCollisionPointTriangle(p, pts[0], pts[1], pts[2]);
	}

	void Move(Vector2 delta) override
	{
		for (int i = 0; i < 3; i++)
			pts[i] = Vector2Add(pts[i], delta);
	}
};

struct LogEvent
{
	double timestamp;
	std::string type;
	Vector2 pos;
	int shapeIndex;
};

std::vector<LogEvent> eventLog;

auto startTime = std::chrono::steady_clock::now();

double GetTimestamp()
{
	auto now = std::chrono::steady_clock::now();
	return std::chrono::duration<double>(now - startTime).count();
}

int main()
{
	eventLog.push_back({GetTimestamp(), "program_start", {0, 0}, -1});

	InitWindow(900, 700, "Trabalho 1 - Programação Avançada");
	SetTargetFPS(60);

	std::vector<std::unique_ptr<Shape>> shapes;
	std::unique_ptr<Shape> current = nullptr;

	int selectedIndex = -1;
	Vector2 prevMouse{};
	bool dragging = false;

	while (!WindowShouldClose())
	{
		Vector2 mouse = GetMousePosition();

		eventLog.push_back({GetTimestamp(), "mouse_move", mouse, -1});

		// criação de formas ao pressionar tecla (somente se não houver uma forma em construção)
		if (!current)
		{
			if (IsKeyPressed(KEY_C))
			{
				eventLog.push_back({GetTimestamp(), "create_circle", mouse, -1});
				current = std::make_unique<CircleShape>();
			}
			else if (IsKeyPressed(KEY_P))
			{
				eventLog.push_back({GetTimestamp(), "create_point", mouse, -1});
				current = std::make_unique<PointShape>();
			}
			else if (IsKeyPressed(KEY_L))
			{
				eventLog.push_back({GetTimestamp(), "create_line", mouse, -1});
				current = std::make_unique<LineShape>();
			}
			else if (IsKeyPressed(KEY_R))
			{
				eventLog.push_back({GetTimestamp(), "create_rectangle", mouse, -1});
				current = std::make_unique<RectShape>();
			}
			else if (IsKeyPressed(KEY_T))
			{
				eventLog.push_back({GetTimestamp(), "create_triangle", mouse, -1});
				current = std::make_unique<TriangleShape>();
			}
		}

		// BACKSPACE cancela forma atual em construção
		if (IsKeyPressed(KEY_BACKSPACE) && current)
		{
			eventLog.push_back({GetTimestamp(), "cancel", mouse, -1});
			current.reset();
		}

		// se há forma em construção, processa input dela
		if (current)
		{
			if (current->HandleInput())
			{
				// assim que ficar pronta move para a lista de shapes
				eventLog.push_back({GetTimestamp(), "shape_ready", mouse, (int)shapes.size()});
				shapes.push_back(std::move(current));
			}
		}
		else
		{
			// seleção (clique esquerdo em forma já pronta, se não estiver criando)
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				selectedIndex = -1;
				for (int i = (int)shapes.size() - 1; i >= 0; i--)
				{
					if (shapes[i]->Contains(mouse))
					{
						eventLog.push_back({GetTimestamp(), "select", mouse, i});
						selectedIndex = i;
						prevMouse = mouse;
						dragging = true;
						break;
					}
				}
			}

			// arrastar
			if (dragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && selectedIndex != -1)
			{
				eventLog.push_back({GetTimestamp(), "drag", mouse, selectedIndex});
				Vector2 delta = Vector2Subtract(mouse, prevMouse);
				shapes[selectedIndex]->Move(delta);
				prevMouse = mouse;
			}
			if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
			{
				eventLog.push_back({GetTimestamp(), "drag_end", mouse, selectedIndex});
				dragging = false;
			}

			// remover
			if (selectedIndex != -1 && IsKeyPressed(KEY_DELETE))
			{
				eventLog.push_back({GetTimestamp(), "remove", mouse, selectedIndex});
				shapes.erase(shapes.begin() + selectedIndex);
				selectedIndex = -1;
			}
		}

		if (IsKeyPressed(KEY_ESCAPE))
		{
			eventLog.push_back({GetTimestamp(), "escape_pressed", mouse, -1});
		}

		// RENDER
		BeginDrawing();
		ClearBackground(RAYWHITE);

		// desenha formas já prontas
		for (int i = 0; i < shapes.size(); i++)
		{
			shapes[i]->Draw();
			if (i == selectedIndex)
			{
				DrawText("Selecionado", 10, 60, 20, RED);
			}
		}

		// desenha a forma em construção (se houver)
		if (current)
			current->Draw();

		// UI
		DrawText("Teclas: C=Circ, P=Ponto, L=Linha, R=Retang, T=Triang, BACKSPACE=Cancelar", 10, 10, 18, DARKGRAY);
		DrawText("Construcao: clique(s) com mouse para definir pontos", 10, 32, 16, DARKGRAY);

		EndDrawing();
	}

	eventLog.push_back({GetTimestamp(), "program_end", {0, 0}, -1});
	CloseWindow();

	std::ofstream logFile("log.csv");
	logFile << "timestamp,event_type,x,y,shape_index\n";
	for (const auto &e : eventLog)
	{
		logFile << e.timestamp << "," << e.type << "," << e.pos.x << "," << e.pos.y << "," << e.shapeIndex << "\n";
	}
	logFile.close();

	double totalTime = GetTimestamp();
	std::ofstream timeFile("execution_time.txt");
	timeFile << "Execution time (s): " << totalTime << "\n";
	timeFile.close();

	return 0;
}
