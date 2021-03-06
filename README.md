<h1>elevators</h1>
<h2>Elevator project, Programming supercomputers</h2>

<h3>Description</h3>
We use two nodes for elevators and three nodes for the floors. When a person stops working, it is sent to a random elevator. 
We use the fact that MPI queues up io requests, so if the elevator is busy, the person just waits.

An elevator transports a person to either of the other floors by sending it to the floor nodes. Only one person can occupy an 
elevator at any moment.


<h3>Status</h3>
<ul>
<li>Model is up and running, persons travel between floors with the elevators</li>
<li>Saves the state correctly to a binary file.</li>
<li>Saves on both floors and in elevator.</li>
<li>Uses relative time to decide when to quit.</li>
<li>Put elevator loop in to a "game loop" like the floor loop/logic. </li>
<li>Names are read from a file in every node, assigned to floor nodes based on rank.</li>
</ul>


<h3>Data file</h3>
Each write consists of 4 bytes.

{id of person}{operation}{elevator}{floor}

<h4>operations</h4>
<ol>
<li>enter elevator on floor</li>
<li>exit elevator on floor</li>
<li>person stops working (sleeping) on floor  (elevator=-1)</li>
</ol>

<h4>Example output</h4>
<pre>
[maa33@login-0-2 elevators]$ od -t d1 data.out 
0000000    6    3   -1    2    6    1    0    4    6    2    0    3    1    3   -1    0  
0000020    5    3   -1    2    2    3   -1    0    1    1    0    2    2    1    1    2  
0000040    1    2    0    4    2    2    1    4    5    1    1    4    5    2    1    3  
0000060    3    3   -1    1    4    3   -1    1    3    1    0    3    3    2    0    4  
0000100    4    1    0    3    4    2    0    2    6    3   -1    1    6    1    0    3  
0000120    6    2    0    4    1    3   -1    2    1    1    1    4    1    2    1    2  
0000140    2    3   -1    2    5    3   -1    1    2    1    1    4    5    1    0    3  
0000160    2    2    1    2    5    2    0    2    4    3   -1    0    1    3   -1    0  
0000200    3    3   -1    2    6    3   -1    2    4    1    1    2    3    1    0    4  
0000220    4    2    1    3    3    2    0    2    1    1    0    2    1    2    0    4
0000240    6    1    0    4    6    2    0    2    2    3   -1    0    2    1    0    2
0000260    2    2    0    3    5    3   -1    0    3    3   -1    0    3    1    0    2
0000300    5    1    1    2    3    2    0    4    5    2    1    4    1    3   -1    2
0000320    1    1    1    4    1    2    1    2    4    3   -1    1    6    3   -1    0
0000340    4    1    1    3    6    1    0    2    4    2    1    2    6    2    0    4
0000360    3    3   -1    2    5    3   -1    2    3    1    1    4    3    2    1    3
0000400    5    1    1    4    5    2    1    3    2    3   -1    1    4    3   -1    0
0000420    2    1    1    3    2    2    1    2    4    1    1    2    4    2    1    4
0000440    1    3   -1    0    1    1    0    2    1    2    0    3    5    3   -1    1
0000460    5    1    1    3    5    2    1    4    3    3   -1    1    6    3   -1    2
0000500    6    1    0    4    3    1    1    3    6    2    0    3    3    2    1    2
0000520    4    3   -1    2    2    3   -1    0    4    1    0    4    4    2    0    2
0000540    2    1    0    2    2    2    0    4    1    3   -1    1    5    3   -1    2
0000560    5    1    1    4    1    1    0    3    5    2    1    2    1    2    0    4
0000600    2    1    1    3    6    2    0    2    2    2    1    2    5    3   -1    1
0000620    3    3   -1    1    5    1    1    3    5    2    1    4    3    1    1    3
0000640    3    2    1    2
0000644
</pre>

<h4>interpretation</h4>
Use a python scipt to read the binary file and name file. Then output a readable log.
<pre>
>>> while True:
...     chunk = f.read(4)
...     if not chunk: break
...     print struct.unpack('bbbb', chunk)
</pre>
