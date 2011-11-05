Vytvorenie nového grafu:
	-treba poui metódy v triede Graph, addNode, addEdge a pod.

Spustenie Layoutovacieho algoritmu
	-spravil som vzorovy priklad, v triede Example sa vytvara vzorovy graf
	-spustanie je v metode AppCore::Core::Core(QApplication * app)

--------------------------------------------------------------------------------------------
Prevedené úpravy:
	-odstránené moduly a triedy, ktoré slúili len na zobrazovanie, GUI a pod.
	-odstránené referencie na OSG, väèšinou boli len zakomentované
	-náhrada za triedu OSG Vec3f, Vec3 a Vec3d je trieda Data::Vector, prebraná z OSG Vec3f
--------------------------------------------------------------------------------------------