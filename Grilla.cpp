
#include <iostream>
#include <vector>
#include <cstdlib>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <cmath>
#include <queue>
#include <algorithm>
#include <utility>

constexpr int GRID_COLS = 80;       // Columnas de la grilla
constexpr int GRID_ROWS = 42;        // Filas de la grilla
constexpr float PANEL_WIDTH = 280.0f; // Ancho del panel lateral
float canvasLeftNdc = -0.56f;  
float canvasWidthNdc = 1.56f;
float canvasTopNdc = 1.0f;
float canvasHeightNdc = 2.0f;

float coordenadaCeldaX(int columna)
{
	return canvasLeftNdc + ((columna + 0.5f) / static_cast<float>(GRID_COLS)) * canvasWidthNdc;
}

float coordenadaCeldaY(int fila)
{
	return canvasTopNdc - ((fila + 0.5f) / static_cast<float>(GRID_ROWS)) * canvasHeightNdc;
}

// Estado del dibujo
bool dibujando = false;
bool hayPuntoAnterior = false;

int puntoAnteriorX = 0;
int puntoAnteriorY = 0;

// Almacenar los puntos que se van dibujando
std::vector<float> puntosDibujo;

// Estructura para almacenar cada dibujo
struct Dibujo
{
	std::vector<float> puntos;
	ImVec4 color;
};

// Lista de dibujos
std::vector<Dibujo> dibujos;
// Historial para deshacer
std::vector<std::vector<Dibujo>> historialDibujos;
// Guardar el estado actual del lienzo
void guardarHistorial()
{
	historialDibujos.push_back(dibujos);
}
// Guardar la figura mientras se esta creando
std::vector<float> puntosVista;
// Variables para la rotacion
bool modoRotar = false;
bool rotando = false;

int dibujoSeleccionado = -1;

std::vector<float> puntosOriginales;

float anguloInicial = 0.0f;
float centroRotacionX = 0.0f;
float centroRotacionY = 0.0f;


// Color seleccionado actualmente
ImVec4 colorSeleccionado = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
bool modoBorrador = false;
// Herramientas de dibujo
int lapicito = 0;

// Punto donde comienza la figura
int puntitoX = 0;
int puntitoY = 0;

// Estado de la figura
bool dibujandito = false;

// Puntos temporales del poligono libre
std::vector<std::pair<int, int>> puntosPoligonoLibre;


// Shader para ubicar los puntos en pantalla
const char* vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(aPos, 1.0);\n"
"}\0";

// Shader para asignar el color
const char* fragmentShaderSource =
"#version 330 core\n"
"uniform vec4 colorFigura;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"    FragColor = colorFigura;\n"
"}\0";

// Algoritmo de Bresenham
void trazarLinea(int xInicial, int yInicial, int xFinal, int yFinal,
	std::vector<float>& puntos)
{
	int diferenciaX = std::abs(xFinal - xInicial);
	int diferenciaY = std::abs(yFinal - yInicial);

	int avanceX = (xInicial < xFinal) ? 1 : -1;
	int avanceY = (yInicial < yFinal) ? 1 : -1;

	int error = diferenciaX - diferenciaY;

	while (true)
	{
		// Calcular el centro de cada casilla
		float coordenadaX = coordenadaCeldaX(xInicial);

		float coordenadaY = coordenadaCeldaY(yInicial);

		puntos.push_back(coordenadaX);
		puntos.push_back(coordenadaY);
		puntos.push_back(0.0f);

		if (xInicial == xFinal && yInicial == yFinal)
			break;

		int dobleError = 2 * error;

		if (dobleError > -diferenciaY)
		{
			error -= diferenciaY;
			xInicial += avanceX;
		}

		if (dobleError < diferenciaX)
		{
			error += diferenciaX;
			yInicial += avanceY;
		}
	}
}

// Dibujar un rectangulo
void trazarRectangulito(int x1, int y1, int x2, int y2,
	std::vector<float>& puntos)
{
	trazarLinea(x1, y1, x2, y1, puntos);
	trazarLinea(x2, y1, x2, y2, puntos);
	trazarLinea(x2, y2, x1, y2, puntos);
	trazarLinea(x1, y2, x1, y1, puntos);
}

// Dibujar un circulo
void trazarCirculito(int centroX, int centroY, int radio,
	std::vector<float>& puntos)
{
	int x = radio;
	int y = 0;
	int error = 1 - radio;

	while (x >= y)
	{
		int px[] = {
			centroX + x, centroX + y,
			centroX - y, centroX - x,
			centroX - x, centroX - y,
			centroX + y, centroX + x
		};

		int py[] = {
			centroY + y, centroY + x,
			centroY + x, centroY + y,
			centroY - y, centroY - x,
			centroY - x, centroY - y
		};

		for (int i = 0; i < 8; i++)
		{
			if (px[i] >= 0 && px[i] < GRID_COLS &&
				py[i] >= 0 && py[i] < GRID_ROWS)
			{
				trazarLinea(
					px[i], py[i], px[i], py[i], puntos
				);
			}
		}

		y++;

		if (error < 0)
		{
			error += 2 * y + 1;
		}
		else
		{
			x--;
			error += 2 * (y - x) + 1;
		}
	}
}

// Dibujar un triangulo
void trazarTriangulito(int x1, int y1, int x2, int y2,
	std::vector<float>& puntos)
{
	int x3 = x1 - (x2 - x1);
	int y3 = y2;

	trazarLinea(x1, y1, x2, y2, puntos);
	trazarLinea(x2, y2, x3, y3, puntos);
	trazarLinea(x3, y3, x1, y1, puntos);
}

// Dibujar un poligono regular de cinco lados
void trazarPoligonoRegular(int centroX, int centroY, int radio,
	std::vector<float>& puntos)
{
	const int lados = 5;
	if (radio <= 0)
		return;

	int vx[lados];
	int vy[lados];

	const float pi = 3.14159265f;
	for (int i = 0; i < lados; i++)
	{
		float angulo = -pi / 2.0f + (2.0f * pi * i / lados);
		vx[i] = centroX + static_cast<int>(std::round(radio * std::cos(angulo)));
		vy[i] = centroY + static_cast<int>(std::round(radio * std::sin(angulo)));
	}

	for (int i = 0; i < lados; i++)
	{
		int siguiente = (i + 1) % lados;
		trazarLinea(vx[i], vy[i], vx[siguiente], vy[siguiente], puntos);
	}
}

// Girar una figura
void girarDibujo(
	std::vector<float>& puntos,
	const std::vector<float>& puntosOriginales,
	float centroX,
	float centroY,
	float angulo)
{
	float coseno = cos(angulo);
	float seno = sin(angulo);

	puntos = puntosOriginales;

	for (size_t i = 0; i + 2 < puntos.size(); i += 3)
	{
		float x = puntosOriginales[i] - centroX;
		float y = puntosOriginales[i + 1] - centroY;

		puntos[i] = centroX + x * coseno - y * seno;
		puntos[i + 1] = centroY + x * seno + y * coseno;
	}
}
// Rellenar una figura
void rellenarFigura(
	int inicioX, int inicioY,
	std::vector<std::vector<bool>>& bordes,
	std::vector<float>& puntos)
{
	bool visitados[GRID_ROWS][GRID_COLS] = {};

	std::queue<std::pair<int, int>> pendientes;

	pendientes.push({ inicioX, inicioY });
	visitados[inicioY][inicioX] = true;

	int movimientosX[] = { 0, 0, 1, -1 };
	int movimientosY[] = { 1, -1, 0, 0 };

	while (!pendientes.empty())
	{
		int x = pendientes.front().first;
		int y = pendientes.front().second;
		pendientes.pop();

		if (bordes[y][x])
			continue;

		float coordenadaX = coordenadaCeldaX(x);

		float coordenadaY = coordenadaCeldaY(y);

		puntos.push_back(coordenadaX);
		puntos.push_back(coordenadaY);
		puntos.push_back(0.0f);

		for (int i = 0; i < 4; i++)
		{
			int nuevoX = x + movimientosX[i];
			int nuevoY = y + movimientosY[i];

			if (nuevoX >= 0 && nuevoX < GRID_COLS &&
				nuevoY >= 0 && nuevoY < GRID_ROWS &&
				!visitados[nuevoY][nuevoX] &&
				!bordes[nuevoY][nuevoX])
			{
				visitados[nuevoY][nuevoX] = true;
				pendientes.push({ nuevoX, nuevoY });
			}
		}
	}
}
int main()
{
	// Iniciar GLFW
	if (!glfwInit())
	{
		std::cout << "Error al iniciar GLFW" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Crear ventana
	GLFWwindow* window = glfwCreateWindow(
		1280, 800, "Mi Paint", NULL, NULL
	);

	if (window == NULL)
	{
		std::cout << "No se pudo crear la ventana" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMaximizeWindow(window);
	glfwPollEvents();
	glfwMakeContextCurrent(window);

	// Cargar funciones de OpenGL
	if (!gladLoadGL())
	{
		std::cout << "Error al inicializar GLAD" << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	// Ajustar el tamaño de visualizacion
	int anchoPantalla, altoPantalla;
	glfwGetFramebufferSize(window, &anchoPantalla, &altoPantalla);
	glViewport(0, 0, anchoPantalla, altoPantalla);

	// Inicializar Dear ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsLight();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	// Los vertices de la grilla se actualizan segun el tamano del lienzo.
	std::vector<float> vertices;

	// Crear vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	// Crear fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	// Crear programa de shaders
	GLuint shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Buscar la variable de color del shader
	GLint ubicacionColor =
		glGetUniformLocation(shaderProgram, "colorFigura");

	// Crear VAO y VBO de la grilla
	GLuint VAO, VBO;

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(
		0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0
	);

	glEnableVertexAttribArray(0);

	// Crear VAO y VBO para los puntos dibujados
	GLuint VAOPuntos, VBOPuntos;

	glGenVertexArrays(1, &VAOPuntos);
	glGenBuffers(1, &VBOPuntos);

	glBindVertexArray(VAOPuntos);
	glBindBuffer(GL_ARRAY_BUFFER, VBOPuntos);

	glBufferData(
		GL_ARRAY_BUFFER,
		0,
		NULL,
		GL_DYNAMIC_DRAW
	);

	glVertexAttribPointer(
		0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0
	);

	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Mantener abierta la ventana
	while (!glfwWindowShouldClose(window))
	{
		int anchoVentana, altoVentana;
		glfwGetWindowSize(window, &anchoVentana, &altoVentana);
		int anchoFramebuffer, altoFramebuffer;
		glfwGetFramebufferSize(window, &anchoFramebuffer, &altoFramebuffer);
		glViewport(0, 0, anchoFramebuffer, altoFramebuffer);
		canvasLeftNdc = -1.0f + (2.0f * PANEL_WIDTH / static_cast<float>(anchoVentana));
		canvasWidthNdc = 2.0f - (2.0f * PANEL_WIDTH / static_cast<float>(anchoVentana));
		canvasTopNdc = 1.0f;
		canvasHeightNdc = 2.0f;

		// Iniciar nuevo fotograma de ImGui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Panel lateral de herramientas
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(PANEL_WIDTH, static_cast<float>(altoVentana)), ImGuiCond_Always);
		ImGui::Begin("Herramientas", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoSavedSettings);
		ImGui::TextUnformatted("Herramientas");
		ImGui::Separator();

		auto seleccionarHerramienta = [&](const char* nombre, int valor) {
			if (ImGui::RadioButton(nombre, lapicito == valor && !modoBorrador && !modoRotar)) {
				lapicito = valor; modoBorrador = false; modoRotar = false;
				dibujandito = false; dibujando = false; hayPuntoAnterior = false;
			}
			};
		seleccionarHerramienta("Lapiz", 0);
		seleccionarHerramienta("Linea", 1);
		seleccionarHerramienta("Rectangulo", 2);
		seleccionarHerramienta("Triangulo", 4);
		seleccionarHerramienta("Poligono regular", 6);
		if (ImGui::RadioButton("Poligono libre", lapicito == 7 && !modoBorrador && !modoRotar)) {
			lapicito = 7; modoBorrador = false; modoRotar = false; puntosPoligonoLibre.clear(); dibujandito = false; dibujando = false;
		}
		seleccionarHerramienta("Circulo", 3);
		if (ImGui::RadioButton("Borrador", modoBorrador)) { modoBorrador = true; modoRotar = false; dibujandito = false; dibujando = false; }
		if (ImGui::RadioButton("Rellenar figura", lapicito == 5 && !modoBorrador && !modoRotar)) { lapicito = 5; modoBorrador = false; modoRotar = false; dibujandito = false; dibujando = false; }
		if (ImGui::RadioButton("Rotar", modoRotar)) { modoRotar = true; modoBorrador = false; dibujandito = false; dibujando = false; hayPuntoAnterior = false; }

		ImGui::Separator();
		ImGui::TextUnformatted("Color");
		if (ImGui::ColorButton("Blanco", ImVec4(1, 1, 1, 1), 0, ImVec2(24, 24))) colorSeleccionado = ImVec4(1, 1, 1, 1);
		ImGui::SameLine();
		if (ImGui::ColorButton("Rojo", ImVec4(1, 0, 0, 1), 0, ImVec2(24, 24))) colorSeleccionado = ImVec4(1, 0, 0, 1);
		ImGui::SameLine();
		if (ImGui::ColorButton("Azul", ImVec4(0, 0, 1, 1), 0, ImVec2(24, 24))) colorSeleccionado = ImVec4(0, 0, 1, 1);
		ImGui::SameLine();
		if (ImGui::ColorButton("Verde", ImVec4(0, 1, 0, 1), 0, ImVec2(24, 24))) colorSeleccionado = ImVec4(0, 1, 0, 1);
		if (ImGui::ColorButton("Amarillo", ImVec4(1, 1, 0, 1), 0, ImVec2(24, 24))) colorSeleccionado = ImVec4(1, 1, 0, 1);

		ImGui::Separator();
		if (ImGui::Button("Deshacer", ImVec2(-1, 0))) { if (!historialDibujos.empty()) { dibujos = historialDibujos.back(); historialDibujos.pop_back(); } }
		if (ImGui::Button("borrar todo el lienzo", ImVec2(-1, 0))) { guardarHistorial(); dibujos.clear(); hayPuntoAnterior = false; }
		ImGui::End();

		// Limpiar el fondo
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Obtener posicion del mouse
		double posicionX, posicionY;
		glfwGetCursorPos(window, &posicionX, &posicionY);

		// Detectar el boton izquierdo
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			if (!ImGui::GetIO().WantCaptureMouse || dibujando || dibujandito)
			{
				if (posicionX >= PANEL_WIDTH && posicionX < anchoVentana &&
					posicionY >= 0 && posicionY < altoVentana)
				{
					int columna = static_cast<int>(
						(posicionX - PANEL_WIDTH) / ((anchoVentana - PANEL_WIDTH) / static_cast<double>(GRID_COLS))
						);

					int fila = static_cast<int>(
						posicionY / (altoVentana / static_cast<double>(GRID_ROWS))
						);
					// Dibujar poligono libre manteniendo presionado el mouse
					if (lapicito == 7)
					{
						if (!dibujandito)
						{
							guardarHistorial();
							puntosPoligonoLibre.clear();
							dibujandito = true;
						}
						if (puntosPoligonoLibre.empty() ||
							puntosPoligonoLibre.back().first != columna ||
							puntosPoligonoLibre.back().second != fila)
						{
							puntosPoligonoLibre.push_back({ columna, fila });
						}
						puntosVista.clear();
						for (size_t k = 1; k < puntosPoligonoLibre.size(); k++)
						{
							trazarLinea(
								puntosPoligonoLibre[k - 1].first,
								puntosPoligonoLibre[k - 1].second,
								puntosPoligonoLibre[k].first,
								puntosPoligonoLibre[k].second,
								puntosVista
							);
						}
					}
					// Seleccionar y girar una figura
					else if (modoRotar)
					{
						if (!rotando)
						{
							dibujoSeleccionado = -1;

							float coordenadaX =
								coordenadaCeldaX(columna);

							float coordenadaY = coordenadaCeldaY(fila);

							for (int i = static_cast<int>(dibujos.size()) - 1; i >= 0; i--)
							{
								for (size_t j = 0; j + 2 < dibujos[i].puntos.size(); j += 3)
								{
									if (std::abs(dibujos[i].puntos[j] - coordenadaX) < (1.0f / GRID_COLS) &&
										std::abs(dibujos[i].puntos[j + 1] - coordenadaY) < (1.0f / GRID_ROWS))
									{
										dibujoSeleccionado = i;
										break;
									}
								}

								if (dibujoSeleccionado != -1)
									break;
							}

							if (dibujoSeleccionado != -1)
							{
								guardarHistorial();
								puntosOriginales = dibujos[dibujoSeleccionado].puntos;

								float minimoX = 1.0f;
								float maximoX = -1.0f;
								float minimoY = 1.0f;
								float maximoY = -1.0f;

								for (size_t i = 0; i + 2 < puntosOriginales.size(); i += 3)
								{
									minimoX = std::min(minimoX, puntosOriginales[i]);
									maximoX = std::max(maximoX, puntosOriginales[i]);

									minimoY = std::min(minimoY, puntosOriginales[i + 1]);
									maximoY = std::max(maximoY, puntosOriginales[i + 1]);
								}

								centroRotacionX = (minimoX + maximoX) / 2.0f;
								centroRotacionY = (minimoY + maximoY) / 2.0f;

								float mouseX = canvasLeftNdc + ((posicionX - PANEL_WIDTH) / (anchoVentana - PANEL_WIDTH)) * canvasWidthNdc;
								float mouseY = canvasTopNdc - (posicionY / altoVentana) * canvasHeightNdc;

								anguloInicial = atan2(
									mouseY - centroRotacionY,
									mouseX - centroRotacionX
								);

								rotando = true;
							}
						}

						if (rotando && dibujoSeleccionado != -1)
						{
							float mouseX = canvasLeftNdc + ((posicionX - PANEL_WIDTH) / (anchoVentana - PANEL_WIDTH)) * canvasWidthNdc;
							float mouseY = canvasTopNdc - (posicionY / altoVentana) * canvasHeightNdc;

							float anguloActual = atan2(
								mouseY - centroRotacionY,
								mouseX - centroRotacionX
							);

							float diferenciaAngulo = anguloActual - anguloInicial;

							girarDibujo(
								dibujos[dibujoSeleccionado].puntos,
								puntosOriginales,
								centroRotacionX,
								centroRotacionY,
								diferenciaAngulo
							);
						}
					}
					// Herramienta borrador
					else if (modoBorrador)
					{
						float coordenadaX =
							coordenadaCeldaX(columna);

						float coordenadaY = coordenadaCeldaY(fila);

						for (int i = 0; i < dibujos.size(); i++)
						{
							std::vector<float>& puntos = dibujos[i].puntos;

							for (size_t j = 0; j + 2 < puntos.size();)
							{
								if (std::abs(puntos[j] - coordenadaX) < 0.0001f &&
									std::abs(puntos[j + 1] - coordenadaY) < 0.0001f)
								{
									puntos.erase(
										puntos.begin() + j,
										puntos.begin() + j + 3
									);
								}
								else
								{
									j += 3;
								}
							}
						}
					}

					// Herramienta lapiz
					else if (lapicito == 0)
					{
						dibujando = true;

						if (!hayPuntoAnterior)
						{
							guardarHistorial();
							Dibujo nuevoDibujo;
							nuevoDibujo.color = colorSeleccionado;

							dibujos.push_back(nuevoDibujo);

							trazarLinea(
								columna, fila,
								columna, fila,
								dibujos.back().puntos
							);
						}
						else
						{
							trazarLinea(
								puntoAnteriorX, puntoAnteriorY,
								columna, fila,
								dibujos.back().puntos
							);
						}

						puntoAnteriorX = columna;
						puntoAnteriorY = fila;
						hayPuntoAnterior = true;
					}
					// Herramienta rellenar
					else if (lapicito == 5)
					{
						if (!dibujandito)
						{
							dibujandito = true;

							std::vector<std::vector<bool>> bordes(
								GRID_ROWS, std::vector<bool>(GRID_COLS, false)
							);

							// Identificar los bordes de los dibujos
							for (int i = 0; i < dibujos.size(); i++)
							{
								std::vector<float>& puntos = dibujos[i].puntos;

								for (size_t j = 0; j + 2 < puntos.size(); j += 3)
								{
									int columnaPunto = static_cast<int>(
										std::round(((puntos[j] - canvasLeftNdc) / canvasWidthNdc) * GRID_COLS - 0.5f)
										);

									int filaPunto = static_cast<int>(
										std::round(((canvasTopNdc - puntos[j + 1]) / canvasHeightNdc) * GRID_ROWS - 0.5f)
										);

									if (columnaPunto >= 0 && columnaPunto < GRID_COLS &&
										filaPunto >= 0 && filaPunto < GRID_ROWS)
									{
										bordes[filaPunto][columnaPunto] = true;
									}
								}
							}

							Dibujo nuevoDibujo;
							nuevoDibujo.color = colorSeleccionado;

							rellenarFigura(
								columna, fila,
								bordes,
								nuevoDibujo.puntos
							);

							if (!nuevoDibujo.puntos.empty())
							{
								guardarHistorial();
								dibujos.push_back(nuevoDibujo);
							}
						}
					}
					// Herramientas de figuras
					else
					{
						if (!dibujandito)
						{
							guardarHistorial();
							puntitoX = columna;
							puntitoY = fila;
							dibujandito = true;
						}

						puntosVista.clear();

						if (lapicito == 1)
						{
							trazarLinea(
								puntitoX, puntitoY,
								columna, fila,
								puntosVista
							);
						}

						if (lapicito == 2)
						{
							trazarRectangulito(
								puntitoX, puntitoY,
								columna, fila,
								puntosVista
							);
						}

						if (lapicito == 3)
						{
							int radio = std::max(
								std::abs(columna - puntitoX),
								std::abs(fila - puntitoY)
							);

							trazarCirculito(
								puntitoX, puntitoY,
								radio,
								puntosVista
							);
						}

						if (lapicito == 4)
						{
							trazarTriangulito(
								puntitoX, puntitoY,
								columna, fila,
								puntosVista
							);
						}

						if (lapicito == 6)
						{
							int radio = std::max(
								std::abs(columna - puntitoX),
								std::abs(fila - puntitoY)
							);
							trazarPoligonoRegular(
								puntitoX, puntitoY, radio, puntosVista
							);
						}
					}
				}
			}
		}
		else
		{
			// Finalizar la rotacion
			if (rotando)
			{
				rotando = false;
				dibujoSeleccionado = -1;
				puntosOriginales.clear();
			}
			// Cerrar el poligono libre al soltar el mouse
			if (lapicito == 7 && dibujandito && puntosPoligonoLibre.size() >= 3)
			{
				puntosVista.clear();
				for (size_t k = 1; k < puntosPoligonoLibre.size(); k++)
				{
					trazarLinea(
						puntosPoligonoLibre[k - 1].first,
						puntosPoligonoLibre[k - 1].second,
						puntosPoligonoLibre[k].first,
						puntosPoligonoLibre[k].second,
						puntosVista
					);
				}
				trazarLinea(
					puntosPoligonoLibre.back().first,
					puntosPoligonoLibre.back().second,
					puntosPoligonoLibre.front().first,
					puntosPoligonoLibre.front().second,
					puntosVista
				);
			}

			// Guardar la figura cuando soltamos el mouse
			if (dibujandito && !puntosVista.empty())
			{

				Dibujo nuevoDibujo;
				nuevoDibujo.color = colorSeleccionado;
				nuevoDibujo.puntos = puntosVista;

				dibujos.push_back(nuevoDibujo);
			}

			dibujando = false;
			dibujandito = false;
			hayPuntoAnterior = false;
			puntosVista.clear();
			if (lapicito == 7)
				puntosPoligonoLibre.clear();
		}

		// Activar el programa de shaders
		glUseProgram(shaderProgram);

		// Construir la grilla en el area disponible a la derecha del panel.
		vertices.clear();
		for (int c = 0; c <= GRID_COLS; ++c)
		{
			float x = canvasLeftNdc + (canvasWidthNdc * c / GRID_COLS);
			vertices.push_back(x); vertices.push_back(-1.0f); vertices.push_back(0.0f);
			vertices.push_back(x); vertices.push_back(canvasTopNdc); vertices.push_back(0.0f);
		}
		for (int r = 0; r <= GRID_ROWS; ++r)
		{
			float y = -1.0f + (canvasHeightNdc * r / GRID_ROWS);
			vertices.push_back(canvasLeftNdc); vertices.push_back(y); vertices.push_back(0.0f);
			vertices.push_back(1.0f); vertices.push_back(y); vertices.push_back(0.0f);
		}
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);

		// Dibujar la grilla
		glUniform4f(
			ubicacionColor,
			0.65f, 0.65f, 0.65f, 1.0f
		);

		glBindVertexArray(VAO);

		glDrawArrays(
			GL_LINES,
			0,
			static_cast<GLsizei>(vertices.size() / 3)
		);

		// Dibujar todos los trazos guardados
		glPointSize(18.0f);

		glBindVertexArray(VAOPuntos);

		for (int i = 0; i < dibujos.size(); i++)
		{
			// Actualizar los puntos del dibujo
			glBindBuffer(GL_ARRAY_BUFFER, VBOPuntos);

			glBufferData(
				GL_ARRAY_BUFFER,
				static_cast<GLsizeiptr>(
					dibujos[i].puntos.size() * sizeof(float)
					),
				dibujos[i].puntos.data(),
				GL_DYNAMIC_DRAW
			);

			// Aplicar el color correspondiente
			glUniform4f(
				ubicacionColor,
				dibujos[i].color.x,
				dibujos[i].color.y,
				dibujos[i].color.z,
				dibujos[i].color.w
			);

			// Mostrar los puntos
			glDrawArrays(
				GL_POINTS,
				0,
				static_cast<GLsizei>(dibujos[i].puntos.size() / 3)
			);
		}

		// Mostrar la figura mientras se mantiene presionado el mouse
		if (dibujandito && !puntosVista.empty())
		{
			glBindBuffer(GL_ARRAY_BUFFER, VBOPuntos);

			glBufferData(
				GL_ARRAY_BUFFER,
				static_cast<GLsizeiptr>(
					puntosVista.size() * sizeof(float)
					),
				puntosVista.data(),
				GL_DYNAMIC_DRAW
			);

			glUniform4f(
				ubicacionColor,
				colorSeleccionado.x,
				colorSeleccionado.y,
				colorSeleccionado.z,
				colorSeleccionado.w
			);

			glPointSize(18.0f);
			glBindVertexArray(VAOPuntos);

			glDrawArrays(
				GL_POINTS,
				0,
				static_cast<GLsizei>(puntosVista.size() / 3)
			);
		}


		// Renderizar interfaz de ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Actualizar ventana
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Cerrar Dear ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Liberar recursos
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	glDeleteVertexArrays(1, &VAOPuntos);
	glDeleteBuffers(1, &VBOPuntos);

	glDeleteProgram(shaderProgram);

	// Cerrar ventana
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}