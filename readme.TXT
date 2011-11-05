Vytvorenie nového grafu:
	-treba poui metódy v triede Graph, addNode, addEdge a pod.

Spustenie Layoutovacieho algoritmu
	-v triede Core treba zavola metódu void AppCore::Core::restartLayout()
	vyznaèil som v nej miesto kde bolo priamo volanie danej metódy triedy FRAlgorithm

--------------------------------------------------------------------------------------------
Prevedené úpravy:
	-odstránené moduly a triedy, ktoré slúili len na zobrazovanie, GUI a pod.
	-odstránené referencie na OSG, väèšinou boli len zakomentované
	-náhrada za triedu OSG Vec3f, Vec3 a Vec3d je trieda Data::Vector, prebraná z OSG Vec3f