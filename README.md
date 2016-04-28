##"Improving Retrieval System Performance with A Domain Algebra"##
My M.Sc. Diploma thesis submitted to the Swiss Federal Institute of Technology (Zurich), back in 1989

From the introduction:

>"In his recently published dissertation, Peter Schäuble defines the mathematical background and a construction process for a
>new information structure, called **domain algebra**, which is supposed to improve the effectiveness of an information
>retrieval system. While it was shown that the construction process always yields an improvement, the magnitude of the 
>improvement and therefore the usefulness of the method was yet unknown. The subject of this diploma thesis is the **actual
>implementation and evaluation of the construction process**."

There are several notable aspects about the system I implemented. Because of the limited memory and performance (by today's
standards) of the computing hardware this code was written for, I had to devise fairly innovative approaches to
implement and adapt various theoretical algorithms to work with the large data collections used for this thesis. A point in case is the computationally very expensive [Simplex optimization routine](https://github.com/ghoss/MSc-Thesis/blob/master/1989/simplex.c)), which I had modified heavily over the course of 
several weeks to accomodate the available resources of one of the Insitute's largest servers. 

The advances and techniques resulting from the final implementation of this domain algebra constructions system significantly facilitated further research in the area at the Swiss Federal Institute of Technology.

I am now making the source code of my thesis available under the **GPLv3 license** for the enjoyment and education of the generations after me.

Guido Hoss, April 2016
