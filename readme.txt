This dataset is  showing a very different behaviour from previous Mynydd Rhiw wide-beam campaign

The most important findings I see are:
1.	The terrestrial layer (Band 3 especially) is carrying a surprisingly large proportion of the service at 500 m altitude. 
2.	Band 40 A2G provides excellent local performance close to A2G sites (high SINR), but exhibits large coverage gaps between sites. 
3.	Accessibility appears to be a larger problem than retainability. 
4.	Short-call performance is significantly worse than long-call performance. 
5.	The uplink appears more vulnerable than the downlink. 
6.	The Minch and the Hebrides remain the most challenging coverage areas. 
7.	Band 3 consistently outperforms Band 20 operationally despite Band 20 often providing stronger RSRP. 
________________________________________
Assessment
Overall RF Assessment
The flight trials demonstrate that LTE service across the Scottish Islands is primarily sustained by the terrestrial network layer, particularly Band 3, despite the aircraft operating at approximately 500 m altitude.
Median RSRP values of approximately -108 to -110 dBm indicate generally moderate coverage availability, while median SINR values between 3 dB and 6 dB demonstrate that service is largely interference- and geometry-limited rather than coverage-limited.
Band 40 A2G cells consistently provide the strongest RF performance when the aircraft is within the effective footprint of an A2G site, frequently achieving SINR values above 10 dB and occasionally exceeding 20 dB. However, these strong localised coverage areas are separated by substantial coverage gaps, limiting the ability of the A2G layer to provide continuous service across the route.
The terrestrial Band 3 layer provides the most consistent radio performance, with relatively stable RSRP, SINR and MOS behaviour across much of the route. Band 20 provides stronger received signal levels but exhibits larger coverage discontinuities and poorer operational performance.
BLER performance is generally good throughout the campaign, although degradation is observed primarily over water crossings, particularly within the Minch and the Hebrides. Missing PUSCH and PDSCH BLER samples in these regions indicate periods where uplink and downlink transport channels could not be consistently maintained.
The significantly larger number of detected terrestrial PCIs compared with A2G PCIs demonstrates the large radio horizon available at 500 m altitude and the resulting complexity of the mobility environment.
________________________________________
Operational Assessment
The operational results suggest that accessibility is the dominant service limitation across the route.
This conclusion is supported by the substantial difference between the long-call and short-call results.
In both serials:
•	Long continuous calls generally remained connected once established. 
•	Short intermittent calls experienced significantly higher setup failure rates. 
•	The increase in setup failures was substantially greater than the increase in call drops. 
This indicates that network attachment and call establishment are more challenging than maintaining an existing VoLTE session.
The behaviour is particularly evident in Serial 1.1:
Metric	UE1 Long Call	UE2 Short Calls
Call Drops	11	9
Setup Fails	19	39
The near doubling of setup failures while operating over the same route strongly suggests an accessibility limitation rather than a retainability limitation.
Similarly, in Serial 1.2:
Metric	Long Call	Short Calls
Call Drops	4	Low
Setup Fails	4	14
The same trend is repeated.
The operational performance is poorest during sea crossings, particularly:
•	The Minch 
•	The Hebrides 
•	Southern Inner Hebrides 
•	Islay/Jura region 
•	Arran/Holy Isle region 
These areas consistently show:
•	Missing MOS 
•	Increased setup failures 
•	Occasional call drops 
•	Missing BLER samples 
•	Reduced signal quality 
The results suggest that service degradation is primarily associated with transitions between coverage regions rather than poor performance within established coverage footprints.
________________________________________
Conclusions
Conclusion 1 – Band 3 is the dominant operational layer
Despite operation at approximately 500 m altitude, Band 3 provides the most consistent service continuity, RF quality and VoLTE performance throughout the route.
Band 3 frequently outperforms Band 20 operationally despite Band 20 occasionally providing stronger received signal levels.
________________________________________
Conclusion 2 – A2G Layer provides strong local performance but insufficient route continuity
Band 40 A2G sites deliver excellent RF quality when the aircraft is located within the intended coverage footprint of individual sites.
However, the measurements demonstrate significant coverage gaps between A2G sites, preventing the A2G layer from providing continuous standalone coverage across the route.
________________________________________
Conclusion 3 – Accessibility is a greater issue than retainability
The consistently higher setup failure rates observed during short-call testing indicate that call establishment is more challenging than maintaining an existing call.
This is one of the strongest findings of the campaign.
________________________________________
Conclusion 4 – Uplink performance is more vulnerable than downlink performance
Missing PUSCH BLER samples occur more frequently than missing PDSCH BLER samples and are concentrated within the same geographic regions where MOS degradation and setup failures occur.
This suggests that uplink limitations are a significant contributor to operational performance degradation.
________________________________________
Conclusion 5 – Water crossings remain the principal challenge
The Minch and Hebrides consistently exhibit:
•	Poor MOS 
•	Missing MOS 
•	BLER degradation 
•	Setup failures 
•	Call drops 
These regions represent the primary operational coverage challenge within the tested route.
________________________________________
Conclusion 6 – RF coverage does not always translate into service availability
Several areas exhibit acceptable RSRP and reasonable SINR while still experiencing poor MOS and elevated setup failures.
This demonstrates that operational VoLTE performance is influenced by accessibility, uplink robustness and mobility behaviour in addition to traditional RF coverage metrics.
________________________________________
Recommendations
1. Accessibility Investigation
Perform a dedicated accessibility analysis focusing on:
•	SIP registration behaviour 
•	VoLTE setup success rate 
•	RRC establishment success 
•	EPS bearer establishment 
•	Setup failure locations 
This appears to be the largest operational issue identified by the campaign.
________________________________________
2. Investigate Uplink Performance
Perform detailed analysis of:
•	PUSCH BLER 
•	Uplink SINR 
•	Uplink scheduling failures 
•	UL Tx Power saturation 
•	Missing uplink measurement regions 
Particularly over:
•	The Minch 
•	The Hebrides 
________________________________________
3. Evaluate A2G Inter-Site Coverage Continuity
Assess:
•	A2G site overlap 
•	Coverage gaps between A2G sites 
•	Inter-site transition regions 
•	Neighbour relations 
The current results indicate strong site-level performance but insufficient route-level continuity.
________________________________________
4. Prioritise Band 3 Operational Role
The terrestrial Band 3 layer appears to provide substantial operational value at 500 m altitude and should be considered an important contributor to service continuity along this route.
________________________________________
5. Targeted Optimisation
Prioritise detailed investigation of:
•	The Minch 
•	Outer Hebrides crossings 
•	Southern Inner Hebrides 
•	Islay/Jura 
•	Arran/Holy Isle 
These regions consistently appear as operational hot-spots.
________________________________________
Suggested PowerPoint Slide Structure
Slide 1 – Campaign Overview
•	Scottish Islands A2G Coverage Assessment 
•	500 m altitude 
•	Two UEs: 
o	Continuous VoLTE call 
o	Short intermittent calls 
•	Routes: 
o	Serial 1.1 Prestwick → Inverness 
o	Serial 1.2 Inverness → Prestwick 
________________________________________
Slide 2 – Key Findings
•	Band 3 provides most consistent operational performance 
•	Band 40 delivers excellent local SINR but large coverage gaps 
•	Accessibility issues dominate over retainability issues 
•	Uplink more vulnerable than downlink 
•	Water crossings remain most challenging 
________________________________________
Slide 3 – Accessibility vs Retainability
•	Short-call setup failures significantly exceed long-call failures 
•	Serial 1.1: 
o	19 vs 39 setup fails 
•	Serial 1.2: 
o	4 vs 14 setup fails 
•	Indicates accessibility limitations rather than call-retention problems 
________________________________________
Slide 4 – Band Performance Comparison
Band	RF Quality	Coverage Continuity	MOS
B3	Good	Best	Best
B20	Moderate	Variable	Poorer
B40	Excellent near site	Large gaps	Variable
________________________________________
Slide 5 – Coverage Hotspots
•	The Minch 
•	Outer Hebrides crossings 
•	Southern Inner Hebrides 
•	Islay/Jura 
•	Arran/Holy Isle 
Common symptoms:
•	Missing MOS 
•	Setup failures 
•	BLER degradation 
•	Call drops 
________________________________________
Slide 6 – Conclusions & Recommendations
•	Band 3 currently provides most reliable route coverage 
•	A2G network provides strong local performance but limited continuity 
•	Accessibility is the primary operational issue 
•	Investigate uplink robustness and A2G inter-site coverage gaps
