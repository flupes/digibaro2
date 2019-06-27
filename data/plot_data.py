import pandas as pds

import matplotlib.pyplot as plt

data = pds.DataFrame.from_csv('samples_with_header.csv')
print(data.index.size)
print(data.head(12))
#print(data.axes)
print(data.tail(12))

dt = data.drop_duplicates(subset=['hour_twelfth'])
print(dt.index.size)
print(dt.head(12))

# 1554073200 --> 2019-03-31 16:00
# 2019-04-14 23:12
# re-create series because we collected data every minute, but the 
# timestamp resolution was 5 minutes

pressure=data['pressure'].values
ps=pds.Series(pressure)

count=0
avg5=0
pavg=[]
for p in pressure:
  avg5 = avg5 + p
  count = count + 1
  if count == 5:
    pavg.append(avg5/5)
    count = 0
    avg5=0

temperature=data['temperature'].values
te=pds.Series(temperature)

humidity=data['humidity'].values
hu=pds.Series(humidity)

pressure5=dt['pressure'].values
ps5=pds.Series(pressure5)

psavg=pds.Series(pavg)

fig, (p1, p2, p3, tp, hp) = plt.subplots(5, sharex=True)

p1.plot(ps)
p1.set(ylabel='pressure (hPa) - 1min interval')

p2.plot(ps5)
p2.set(ylabel='pressure (hPa) - 5min interval')

p3.plot(psavg)
p3.set(ylabel='pressure (hPa) - average over 5 samples')

tp.plot(te)
tp.set(ylabel='temperature (deg. C)')

hp.plot(hu)
hp.set(ylabel='humidity (%)', xlabel='time (minutes)')

plt.show()
