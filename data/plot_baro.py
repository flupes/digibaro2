import sys
import pandas as pd
from datetime import datetime

import plotly.graph_objects as go
import plotly.express as px
from plotly.subplots import make_subplots


if __name__ == "__main__":

    if len(sys.argv) < 3:
        print("Usage: python plot_baro.py <filename.csv> <location>")
        sys.exit(1)

    colors = px.colors.qualitative.Dark2

    data = pd.read_csv(sys.argv[1])
    data["datetime"] = pd.to_datetime(data["unix_seconds"], unit="s")

    fig = go.Figure()
    fig.add_trace(
        go.Scatter(
            x=data["datetime"],
            y=data["pressure"],
            line_color=colors[0],
            name="p",
            yaxis="y",
        )
    )
    fig.add_trace(
        go.Scatter(
            x=data["datetime"],
            y=data["temperature"],
            line_color=colors[2],
            name="t",
            yaxis="y2",
        )
    )
    fig.add_trace(
        go.Scatter(
            x=data["datetime"],
            y=data["humidity"],
            line_color=colors[3],
            name="h",
            yaxis="y3",
        )
    )

    fig.update_traces(hoverinfo="x+y", line={"width": 0.7}, mode="lines", showlegend=False)

    fig.update_layout(
        xaxis=dict(
            autorange=False,
            range=["2019-07-14 00:00:00", "2023-01-14 00:00:00"],
            rangeslider=dict(
                autorange=False, range=["2019-07-14 00:00:00", "2023-01-14 00:00:00"]
            ),
            type="date",
        ),
        yaxis=dict(
            anchor="x",
            autorange=True,
            domain=[0.4, 1.0],
            showline=True,
            side="right",
            tickfont={"color": colors[0]},
            titlefont={"color": colors[0]},
            title="pressure (hPa)",
        ),
        yaxis2=dict(
            anchor="x",
            autorange=True,
            domain=[0.2, 0.4],
            showline=True,
            side="right",
            tickfont={"color": colors[2]},
            titlefont={"color": colors[2]},
            title="temperature (deg. C)",
        ),
        yaxis3=dict(
            anchor="x",
            autorange=True,
            domain=[0.0, 0.2],
            showline=True,
            side="right",
            tickfont={"color": colors[3]},
            titlefont={"color": colors[3]},
            title="humidity (%)",
        ),
    )

    fig.update_layout(
        template="plotly_white", title=f"Data collected by the VAI Digibaro located at {sys.argv[2]}"
    )

    fig.show()

    fig.write_html(f"{sys.argv[1].replace('.csv', '.html')}")
