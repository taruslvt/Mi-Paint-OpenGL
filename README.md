# MI PAINT - OPENGL

## 1. Información del estudiante

**Nombre:** Laura Camila Romero Gómez  
**Código:** 6014539

## 2. Descripción del proyecto

Mi Paint es una aplicación de dibujo desarrollada en C++ utilizando OpenGL. Permite al usuario crear dibujos mediante herramientas como lápiz, borrador, relleno de color, selección y rotación, además de generar figuras geométricas como líneas, rectángulos, círculos, triángulos y pentágonos.

La aplicación cuenta con una interfaz gráfica que facilita la selección de herramientas y colores, así como la interacción con el área de dibujo.

## 3. Arquitectura del proyecto

El proyecto está organizado en diferentes componentes que trabajan conjuntamente:

- **Interfaz gráfica:** utiliza Dear ImGui para mostrar el panel de herramientas, los botones de selección de figuras, los colores y las opciones de edición.
- **Gestión de entrada:** procesa los movimientos del mouse y los clics para identificar las acciones realizadas por el usuario dentro del área de dibujo.
- **Algoritmos de dibujo:** contiene funciones para generar líneas mediante el algoritmo de Bresenham y construir figuras geométricas mediante algoritmos matemáticos.
- **Edición de figuras:** permite realizar acciones como borrar, rellenar, seleccionar y rotar los dibujos.
- **Renderizado gráfico:** utiliza OpenGL para representar la cuadrícula, las figuras y los puntos almacenados en pantalla.
- **Gestión de ventanas:** utiliza GLFW para crear y controlar la ventana de la aplicación, mientras GLAD permite acceder a las funciones de OpenGL.

El funcionamiento integra estos componentes para convertir las acciones del usuario en elementos gráficos que se muestran en el lienzo.
