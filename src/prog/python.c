/* prog/python.c */

/* 2017-12-13 - Code for Python integration - QBFreak@qbfreak.net
 *  Python integration via @python, @pytr commands and python() function
 */

/* Python module, handles @python, @pytr commands. */

#include "externs.h"

void do_python(dbref player, char *python) {
    Py_Initialize();

    //const char* pythonScript = "result = multiplicand * multiplier\n";
    PyObject* pmain = PyImport_AddModule("__main__");
    PyObject* globalDictionary = PyModule_GetDict(pmain);
    PyObject* localDictionary = PyDict_New();

    // Redirect stdout
    PyObject* pyStdOut = PyFile_FromString("CONOUT$", "w+");
    PyObject* sys = PyImport_ImportModule("sys");
    PyObject_SetAttrString(sys, "stdout", pyStdOut);

    // PyDict_SetItemString(localDictionary, "multiplicand", PyInt_FromLong(2));
    // PyDict_SetItemString(localDictionary, "multiplier", PyInt_FromLong(5));
    PyRun_String(python, Py_file_input, globalDictionary, localDictionary);

    // long result = PyInt_AsLong(PyDict_GetItemString(localDictionary, "result"));

    //notify(player, "Result: %ld", result);

    // Display output
    FILE* pythonOutput = PyFile_AsFile(pyStdOut);
    rewind(pythonOutput);
    char line[1024];
    while(fgets(line, sizeof line, pythonOutput) != NULL) {
        // Strip trailing \n
        for (int i=0;i<1024;i++) {
            if (line[i] == 0 && i > 0 && line[i-1] == '\n')
                line[i-1] = 0;
        }
        notify(player, "%s", line);
    }

    // Check for errors
    PyObject* ex = PyErr_Occurred();
    if (ex) {
        PyObject *pExcType , *pExcValue , *pExcTraceback ;
        PyErr_Fetch( &pExcType, &pExcValue, &pExcTraceback );
        if (pExcType != NULL) {
            PyObject* pRepr = PyObject_Repr(pExcType);
            log_error("Python exception: %s", PyString_AsString(pRepr));
            Py_DecRef(pRepr);
            Py_DecRef(pExcType);
        }
        if (pExcValue != NULL) {
            PyObject* pRepr = PyObject_Repr(pExcValue);
            log_error("Python exception: %s", PyString_AsString(pRepr));
            Py_DecRef(pRepr);
            Py_DecRef(pExcValue);
        }
        /* Do something with exception */
        notify(player, "#-1 Python exception");
    }

    // Done!
    Py_Finalize();
}

/* Trigger an attribute to run as Python */
void do_pytr(player, object, arg2, argv, pure, cause)
dbref player, cause;
char *object, *arg2, **argv, *pure;
{
  dbref thing;
  ATTR *attr;
  char *s;
  int a;

  /* validate target */
  if(!(s=strchr(object, '/'))) {
    notify(player, "You must specify an attribute name.");
    return;
  }
  *s++='\0';
  if((thing=match_thing(player, object, MATCH_EVERYTHING)) == NOTHING)
    return;
  if(!controls(player, thing, POW_MODIFY)) {
    notify(player, "Permission denied.");
    return;
  }

  /* Validate attribute */
  if(!(attr=atr_str(thing, s))) {
    notify(player, "No match.");
    return;
  } if(!can_see_atr(player, thing, attr)) {
    notify(player, "Permission denied.");
    return;
  } if(attr->flags & (AF_HAVEN|AF_LOCK|AF_FUNC)) {
    notify(player, "Cannot trigger that attribute.");
    return;
  }

  /* Trigger attribute */
  for(a=0;a<10;a++)
    if(argv[a+1])
      strcpy(env[a], argv[a+1]);
    else
      *env[a]='\0';

  if(!Quiet(player))
    notify(player, "%s - Triggered.", db[thing].name);

  char *contents=atr_get_obj(thing, attr->obj, attr->num);
  do_python(player, contents);

}
