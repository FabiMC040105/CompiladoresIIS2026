# 5. Atributos solicitados

Las preguntas se transcriben en su sentido desde el apartado **8.2.1 Documentación** de la especificación del Proyecto Grupal II. Las respuestas son un borrador técnico basado en el código del repositorio; deben revisarse por el equipo antes de la entrega.

## 5.1 Atributo de Conocimiento de Ingeniería (CI)

**Pregunta: ¿Cuáles conceptos matemáticos o de ciencias naturales y fundamentos especializados utilizó en la solución del proyecto? Explique con detalle y muestre un ejemplo.**

**Respuesta:** El proyecto utiliza teoría de grafos, análisis de flujo de datos, fundamentos de compiladores y algoritmos. Los bloques básicos constituyen nodos de un grafo de flujo de control (CFG) y sus enlaces muestran las rutas que puede seguir un programa. Sobre el CFG se calcula la información de variables vivas, usada por DCE y por spilling. Por su parte, el grafo de interferencia relaciona dos temporales cuando necesitan coexistir, lo que permite modelar la asignación de registros como un problema de coloreo.

Por ejemplo, supongamos que hay diez temporales vivos al mismo tiempo y solo nueve registros físicos disponibles para temporales. Si el grafo forma un conjunto de diez valores mutuamente interferentes, no puede colorearse con nueve colores. El compilador debe reducir la presión sobre registros almacenando temporalmente algunos valores en memoria y recuperándolos cuando corresponda. En el código, este proceso se encuentra en `liveness.c`, `grafo_interferencia.c` y `spill_transform.c`.

También se emplean relaciones de dependencia **RAW, WAR y WAW** para validar si una instrucción puede cambiar de posición. Estas relaciones son importantes para VLIW, porque el compilador debe decidir el orden antes de la ejecución, sin depender de un mecanismo de calendarización dinámica del hardware.

## 5.2 Persona Ingeniera y el Mundo (PM)

**Pregunta 1: Identifique los aspectos sociales, económicos, legales, ambientales, de seguridad y salud en el análisis y evaluación del impacto ambiental para el desarrollo sostenible al resolver problemas complejos de ingeniería, específicamente en la parte de compiladores.**

**Respuesta:** En la dimensión social, un compilador más eficiente puede mejorar la respuesta de programas usados por muchas personas, aunque el beneficio depende de la aplicación. En el aspecto económico, reducir ciclos, accesos a memoria y tiempo de ejecución puede disminuir costos de operación en sistemas usados repetidamente. En el aspecto ambiental, un menor trabajo computacional **puede** reducir consumo energético, pero debe medirse porque ciertas optimizaciones, como el desenrollado de ciclos, aumentan el tamaño del código y podrían incrementar accesos a memoria o consumo.

Desde la dimensión legal, el desarrollo debe respetar licencias y derechos de autor de herramientas, bibliotecas y aportes del equipo, así como el tratamiento adecuado de archivos del usuario. En seguridad, una optimización nunca debe cambiar las comprobaciones de autorización ni eliminar llamadas con efectos importantes. Respecto a la salud, la mejora de herramientas usadas en contextos críticos requiere validar con especial cuidado que la transformación conserve los resultados y no comprometa la confiabilidad.

**Pregunta 2: Explique la importancia de los aspectos sociales, económicos, legales, ambientales, de seguridad y salud en el análisis y evaluación del impacto ambiental para el desarrollo sostenible al resolver problemas complejos de ingeniería.**

**Respuesta:** Estos aspectos evitan evaluar una solución únicamente por su velocidad. Una transformación que disminuye instrucciones pero genera resultados incorrectos no es una mejora aceptable. Asimismo, un compilador que reduce ciclos en una prueba pequeña podría aumentar el tamaño del programa o el tráfico de memoria en otro caso. Por eso, el impacto debe analizarse con métricas, pruebas de equivalencia y criterios de seguridad, junto con el cumplimiento de las licencias aplicables. Esta evaluación permite equilibrar rendimiento, costo, confiabilidad y uso responsable de recursos.

**Pregunta 3: Explique cómo integrar los aspectos sociales, económicos, legales, ambientales, de seguridad y salud en el análisis y evaluación del impacto ambiental para el desarrollo sostenible al resolver problemas complejos de ingeniería.**

**Respuesta:** Se pueden integrar desde la planificación de pruebas y criterios de aceptación. Para cada optimización conviene conservar una versión sin optimizar, ejecutar pruebas de corrección y registrar instrucciones, tamaño, ciclos y tiempo de compilación. La integración con VLIW debe revisar que se conserven llamadas, accesos a memoria, decisiones de control y las operaciones de seguridad de la ISA. También debe verificarse el uso autorizado de herramientas y bibliotecas, el manejo seguro de archivos del IDE y la trazabilidad de cambios mediante Git. Finalmente, se comparan los resultados y solo se considera beneficiosa una optimización cuando mantiene la semántica y presenta ventajas comprobables para el escenario estudiado.

## 5.3 Relación con las pruebas del repositorio

La batería actual verifica corrección lógica y equivalencia de resultados para distintas transformaciones. Este es un primer paso para analizar confiabilidad, pero todavía falta recopilar métricas suficientes de ciclos y accesos a memoria para sustentar conclusiones cuantitativas sobre rendimiento o ahorro energético.
