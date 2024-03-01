import { MapContainer, Popup, TileLayer, Circle } from 'react-leaflet';
import { useEffect, useState } from 'react';
import 'leaflet/dist/leaflet.css';
import { createClient } from '@supabase/supabase-js';

// Create a single supabase client for interacting with your database
const supabase = createClient(process.env.NEXT_PUBLIC_SUPABASE_URL, process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY);

export default function Map() {
  const [nodes, setNodes] = useState([]);
  const nodeRadius = 200;
  const fireTemperature = 30;

  useEffect(() => {
    const fetchData = async () => {
      const { data, error } = await supabase
        .from('nodes')
        .select();

      if (error) {
        console.error('Error fetching data:', error);
      } else {
        setNodes(data);
      }
    };

    fetchData(); // Call once immediately
    const intervalId = setInterval(fetchData, 10000); // Then every 10 seconds

    return () => clearInterval(intervalId); // Cleanup on component unmount
  }, []); // Empty dependency array means this effect runs once on mount

  return (
    <MapContainer center={[44.230687, -76.481323]} zoom={13} scrollWheelZoom={true} style={{height: "100%", width: "100%"}}>
      <TileLayer
        attribution='&copy; <a href="http://osm.org/copyright">OpenStreetMap</a> contributors'
        url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
      />
      {nodes.map(node => (
        <Circle key={node.id} center={JSON.parse(node.gpsCoordinates)} radius={nodeRadius} pathOptions={node.temperature > fireTemperature ? { color: 'red' } : {}}>
          <Popup>
            <p>Node ID: {node.id}</p>
            <p>Temperature: {node.temperature} °C</p>
            <p>Location: {JSON.parse(node.gpsCoordinates)[0]}, {JSON.parse(node.gpsCoordinates)[1]}</p>
          </Popup>
        </Circle>
      ))}
    </MapContainer>
  );
}
