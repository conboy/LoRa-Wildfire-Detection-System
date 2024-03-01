import { MapContainer, Marker, Popup, TileLayer, Circle } from 'react-leaflet'
import { useState } from 'react'
import 'leaflet/dist/leaflet.css'

const Map = () => {

  const [nodes, setNodes] = useState([{id: 1, temperature: 20, gpsCoordinates: [44.230687, -76.481323]}, {id: 2, temperature: 31, gpsCoordinates: [44.2292189195705, -76.49426302982012]}]);
  const nodeRadius = 200;
  const fireTemperature = 30;

  return (
    <MapContainer center={[44.230687, -76.481323]} zoom={13} scrollWheelZoom={true} style={{height: "100%", width: "100%"}}>
      <TileLayer
        attribution='&copy; <a href="http://osm.org/copyright">OpenStreetMap</a> contributors'
        url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
      />
      {nodes.map(node => (
        <Circle key={node.id} center={node.gpsCoordinates} radius={nodeRadius} pathOptions={node.temperature > fireTemperature ? { color: 'red' } : {}}>
          <Popup>
            <p>Node ID: {node.id}</p>
            <p>Temperature: {node.temperature} °C</p>
            <p>Location: {node.gpsCoordinates[0]}, {node.gpsCoordinates[1]}</p>
          </Popup>
        </Circle>
      ))}
    </MapContainer>
  )
}

export default Map