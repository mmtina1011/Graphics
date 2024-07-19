# Circle Limit

Maurits Cornelis Escher tiszteletére készítsünk egy egyszerű Circle Limit-re hajazó (de annál lényegesen
egyszerűbb) textúrát pixelvezérelt képszintézis algoritmussal, majd az elkészített textúrát felhasználva
objektumvezérelt eljárással jelenítsünk meg egy forgó/keringő négyágú csillagot.

A hiperbolikus síkon keletkező ábrát a Poincaré diszkre vetítve jelenítjük meg

A textúra kezdeti felbontása 300x300-as és GL_LINEAR szűrési módot használunk. Az ‘r’ billentyű
lenyomásának hatására a lineáris felbontás 100-al csökken, az ’R’ hatására 100-al nő. A ‘t’-vel
GL_NEAREST módot állíthatunk, a ‘T’-vel pedig GL_LINEAR-t.

A csillag középpontja egy körpályán mozog az (50, 30) pontból indulva, a teljes kört 10 sec alatt megtéve.
A körpálya középpontja a (20, 30)-as pont és sugara 30. Eközben a csillag a saját középpontja körül forog
0.2 sec-1
fordulatszámmal. A csillag animációja az ‘a’ lenyomásával indítható.