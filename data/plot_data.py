import pandas as pds

import matplotlib.pyplot as plt

data = pds.DataFrame.from_csv('samples_with_header.csv')

# re-create series because we collected data every minute, but the 
# timestamp resolution was 5 minutes

pressure=data['pressure'].values
ps=pds.Series(pressure)

temperature=data['temperature'].values
te=pds.Series(temperature)

humidity=data['humidity'].values
hu=pds.Series(humidity)

fig, (pp, tp, hp) = plt.subplots(3, sharex=True)

pp.plot(ps)
pp.set(ylabel='pressure (hPa)')

tp.plot(te)
tp.set(ylabel='temperature (deg. C)')

hp.plot(hu)
hp.set(ylabel='humidity (%)', xlabel='time (minutes)')

plt.show()
