<!-- Silicon LogiX / SLX SynthForge · Copyright (c) 2026 Marco Pezzullo. -->
# Presentation walkthrough · 0.4.1

1. Open Scene studio, select C2 and remove it. Move U4 and rotate it; several
   components can be edited together. Show Undo/Redo and Ctrl+Z/Ctrl+Y.
2. Start inspection. Select a defect to connect its RGB detail, CAD reference,
   score and AI version. Stop inspection to edit again.
3. Restore the board and reacquire to show a nominal result. Guided trial follows
   the user's actual actions rather than automatically inserting defects.
4. Save an experiment and open the HTML report. Stop, edit and load its folder
   to show that captured pixels and verified results can be reopened.
5. Open Assembly AI lab and click **Show example comparison** to inspect a
   completed run. Explain raw misses, accepted escapes, reviews and coverage.
   Selecting a model is an explicit action.
6. With Python installed, freeze a new independent test, confirm a development
   case in the editor, generate additional examples and train a candidate.
   Compare it on the same frozen data and original control test.
7. In the five-scenario laboratory, show the U4 solder bridge and robustness
   variations. The five-class model is separate from the editable-board inspector.

The completed example is an actual run. Both versions have equal raw accuracy
and missed-defect counts; the candidate has one fewer review. A cycle may improve,
leave performance unchanged or regress. The interface reports all three outcomes.

The demonstration contains original synthetic assets and no customer data.
It does not validate the electrical circuit or real production inspection.
